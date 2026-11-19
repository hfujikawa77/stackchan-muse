// Google Cloud Text-to-Speech client. A worker task splits the reply into short
// sentences, POSTs each to texttospeech.googleapis.com and streams the MP3 that
// comes back into a stream buffer that the Muse session task drains.
#include "stackchan_tts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include "mbedtls/base64.h"
#include "nvs.h"

static const char *TAG = "sc.tts";

#define NS "stackchan"
#define KEY_MAX 80
#define VOICE_MAX 40
#define DEFAULT_VOICE "ja-JP-Neural2-B"
#define FIRST_CHUNK_BYTES 72    // short first piece so speech starts sooner
#define CHUNK_BYTES 260
#define TEXT_LIMIT 6000         // bytes of a reply we will voice
#define RESP_MAX (192 * 1024)
#define SB_SIZE (48 * 1024)

static StreamBufferHandle_t s_sb;
static SemaphoreHandle_t s_lock;
static TaskHandle_t s_task;
static char *s_pending;
static uint32_t s_pending_gen;
static volatile uint32_t s_gen;        // newest request
static volatile uint32_t s_ready_gen;  // request whose buffer state is valid
static volatile bool s_done, s_got;

typedef struct { uint8_t *buf; size_t len, cap; bool over; } resp_t;

#define BASE_URL "https://texttospeech.googleapis.com/"
#define IDLE_DROP_US (45LL * 1000000)   // drop a connection idle this long; the server will have closed it
static esp_http_client_handle_t s_h;   // kept between requests so the TLS handshake is paid once
static resp_t s_r;
static int64_t s_last_us;
static volatile bool s_prewarm;

static bool nvs_get(const char *k, char *out, size_t cap) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) return false;
    size_t n = cap;
    esp_err_t e = nvs_get_str(h, k, out, &n);
    nvs_close(h);
    return e == ESP_OK && out[0];
}

static bool nvs_put(const char *k, const char *v) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t e = v[0] ? nvs_set_str(h, k, v) : nvs_erase_key(h, k);
    if (e == ESP_ERR_NVS_NOT_FOUND) e = ESP_OK;
    if (e == ESP_OK) e = nvs_commit(h);
    nvs_close(h);
    return e == ESP_OK;
}

bool stackchan_tts_enabled(void) {
    char k[KEY_MAX];
    return nvs_get("tts_key", k, sizeof k);
}

bool stackchan_tts_console_command(const char *line) {
    const char *key = "tts_key=", *voice = "tts_voice=";
    if (!strncmp(line, key, strlen(key))) {
        const char *v = line + strlen(key);
        bool ok = strlen(v) < KEY_MAX && nvs_put("tts_key", v);
        if (ok) printf("@tts_key ok (%u chars)\n", (unsigned)strlen(v));
        else printf("@tts_key error\n");
        fflush(stdout);
        return true;
    }
    if (!strncmp(line, voice, strlen(voice))) {
        const char *v = line + strlen(voice);
        bool ok = strlen(v) < VOICE_MAX && nvs_put("tts_voice", v);
        printf(ok ? "@tts_voice ok\n" : "@tts_voice error\n");
        fflush(stdout);
        return true;
    }
    return false;
}

// ---- text handling ----

static int utf8_len(unsigned char c) {
    return c < 0x80 ? 1 : c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : 2;
}

// Drops markdown marks and 4-byte sequences (emoji), which the voice can't read.
static size_t sanitize(const char *in, char *out, size_t cap) {
    size_t n = 0;
    const unsigned char *p = (const unsigned char *)in;
    while (*p && n + 5 < cap) {
        unsigned char c = *p;
        int len = utf8_len(c);
        if (len == 4 || c == '*' || c == '#' || c == '`' || c == '_' || c == '~') {
            for (int i = 0; i < len && *p; i++) p++;
            continue;
        }
        for (int i = 0; i < len && *p; i++) out[n++] = (char)*p++;
    }
    out[n] = 0;
    return n;
}

// Length of the sentence delimiter at p (0 if none).
static int delim_len(const char *p) {
    unsigned char a = (unsigned char)p[0], b = (unsigned char)p[1], c = (unsigned char)p[2];
    if (a == '\n') return 1;
    if ((a == '.' || a == '!' || a == '?') && (p[1] == ' ' || p[1] == '\n' || !p[1])) return 1;
    if (a == 0xE3 && b == 0x80 && c == 0x82) return 3;                       // 。
    if (a == 0xEF && b == 0xBC && (c == 0x81 || c == 0x9F)) return 3;        // ！ ？
    return 0;
}

// Length of the next chunk: at most `limit` bytes, ending at a sentence boundary.
static size_t next_chunk(const char *s, size_t limit) {
    size_t i = 0, last = 0;
    while (s[i]) {
        int d = delim_len(s + i);
        int step = d ? d : utf8_len((unsigned char)s[i]);
        if (i + step > limit) break;
        i += step;
        if (d) last = i;
    }
    if (!s[i]) return i;           // the rest fits
    return last ? last : i;        // else break at a sentence end, or hard-split
}

static void json_escape(const char *in, size_t n, char *out) {
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '"' || c == '\\') { *out++ = '\\'; *out++ = (char)c; }
        else if (c < 0x20) { *out++ = ' '; }
        else *out++ = (char)c;
    }
    *out = 0;
}

// ---- HTTP ----

static esp_err_t on_http(esp_http_client_event_t *e) {
    resp_t *r = e->user_data;
    if (e->event_id == HTTP_EVENT_ON_DATA && r && !r->over) {
        size_t need = r->len + e->data_len + 1;
        if (need > r->cap) {
            size_t cap = need * 2;
            if (cap > RESP_MAX) cap = RESP_MAX;
            if (need > cap) { r->over = true; return ESP_OK; }
            uint8_t *nb = heap_caps_realloc(r->buf, cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!nb) { r->over = true; return ESP_OK; }
            r->buf = nb;
            r->cap = cap;
        }
        memcpy(r->buf + r->len, e->data, e->data_len);
        r->len += e->data_len;
        r->buf[r->len] = 0;
    }
    return ESP_OK;
}

// Pushes MP3 bytes into the stream buffer; false if the request was replaced.
static bool push(uint32_t gen, const uint8_t *d, size_t n) {
    while (n) {
        if (s_gen != gen) return false;
        size_t w = xStreamBufferSend(s_sb, d, n, 0);
        d += w;
        n -= w;
        if (n) vTaskDelay(pdMS_TO_TICKS(20));
    }
    return true;
}

// Fetches one chunk and pushes its MP3. Returns 1 ok, 0 failed, -1 replaced.
static int speak_chunk(esp_http_client_handle_t h, resp_t *r, const char *voice, const char *lang,
                       const char *txt, size_t n, uint32_t gen) {
    size_t bl = n * 2 + 256;
    char *esc = heap_caps_malloc(n * 2 + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *body = heap_caps_malloc(bl, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!esc || !body) { free(esc); free(body); return 0; }
    json_escape(txt, n, esc);
    snprintf(body, bl,
             "{\"input\":{\"text\":\"%s\"},\"voice\":{\"languageCode\":\"%s\",\"name\":\"%s\"},"
             "\"audioConfig\":{\"audioEncoding\":\"MP3\",\"pitch\":4.0}}", esc, lang, voice);
    free(esc);
    r->len = 0;
    r->over = false;
    if (r->buf) r->buf[0] = 0;
    esp_http_client_set_post_field(h, body, (int)strlen(body));
    int64_t t0 = esp_timer_get_time();
    esp_err_t err = esp_http_client_perform(h);
    wifi_ap_record_t ap;
    int rssi = esp_wifi_sta_get_ap_info(&ap) == ESP_OK ? ap.rssi : 0;
    ESP_LOGI(TAG, "chunk %u bytes: %d ms (rssi %d dBm)", (unsigned)n, (int)((esp_timer_get_time() - t0) / 1000), rssi);
    int status = esp_http_client_get_status_code(h);
    free(body);
    if (s_gen != gen) return -1;
    if (err != ESP_OK || status != 200 || r->over || !r->buf) {
        ESP_LOGW(TAG, "synthesize failed: err=%s status=%d len=%u%s", esp_err_to_name(err), status,
                 (unsigned)r->len, r->over ? " (too large)" : "");
        if (status != 200 && r->buf && r->len) ESP_LOGW(TAG, "%.200s", (char *)r->buf);
        return 0;
    }
    char *p = strstr((char *)r->buf, "\"audioContent\"");
    if (p) p = strchr(p + 14, '"');           // opening quote of the value
    if (!p) return 0;
    char *b64 = p + 1;
    char *q = strchr(b64, '"');
    if (!q) return 0;
    size_t b64len = (size_t)(q - b64), olen = 0;
    size_t cap = b64len / 4 * 3 + 4;
    uint8_t *mp3 = heap_caps_malloc(cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!mp3) return 0;
    if (mbedtls_base64_decode(mp3, cap, &olen, (const unsigned char *)b64, b64len) != 0) {
        free(mp3);
        return 0;
    }
    bool ok = push(gen, mp3, olen);
    free(mp3);
    if (!ok) return -1;
    s_got = true;
    return 1;
}

static void drop_client(void) {
    if (s_h) esp_http_client_cleanup(s_h);
    s_h = NULL;
}

// The kept connection, or a new one (not yet connected).
static esp_http_client_handle_t get_client(void) {
    if (s_h && esp_timer_get_time() - s_last_us > IDLE_DROP_US) drop_client();
    if (!s_h) {
        esp_http_client_config_t cfg = {
            .url = BASE_URL, .timeout_ms = 8000, .crt_bundle_attach = esp_crt_bundle_attach,
            .event_handler = on_http, .user_data = &s_r, .buffer_size = 2048, .keep_alive_enable = true,
        };
        s_h = esp_http_client_init(&cfg);
        if (s_h) esp_http_client_set_header(s_h, "Content-Type", "application/json");
    }
    return s_h;
}

// Opens the TLS connection ahead of the first request.
static void warm(void) {
    if (!stackchan_tts_enabled()) return;
    if (s_h && esp_timer_get_time() - s_last_us < 5LL * 1000000) return;   // still warm (Google closes idle connections within seconds)
    esp_http_client_handle_t h = get_client();
    if (!h) return;
    s_r.len = 0;
    s_r.over = false;
    esp_http_client_set_url(h, BASE_URL);
    esp_http_client_set_method(h, HTTP_METHOD_GET);
    int64_t t0 = esp_timer_get_time();
    esp_err_t err = esp_http_client_perform(h);
    s_last_us = esp_timer_get_time();
    ESP_LOGI(TAG, "prewarm: %s, %d ms", esp_err_to_name(err), (int)((s_last_us - t0) / 1000));
    if (err != ESP_OK) drop_client();
}

static void run_request(char *text, uint32_t gen) {
    char key[KEY_MAX], voice[VOICE_MAX] = DEFAULT_VOICE, lang[8] = "ja-JP";
    if (!nvs_get("tts_key", key, sizeof key)) return;
    nvs_get("tts_voice", voice, sizeof voice);
    if (strlen(voice) >= 5 && voice[2] == '-' && voice[5] == '-') {
        memcpy(lang, voice, 5);
        lang[5] = 0;
    }

    char *clean = heap_caps_malloc(TEXT_LIMIT + 8, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!clean) return;
    size_t total = sanitize(text, clean, TEXT_LIMIT);

    char url[160];
    snprintf(url, sizeof url, BASE_URL "v1/text:synthesize?key=%s", key);
    size_t off = 0;
    bool first = true;
    int fails = 0;
    while (off < total && s_gen == gen && fails < 3) {
        while (clean[off] == ' ' || clean[off] == '\n') off++;
        if (off >= total) break;
        size_t n = next_chunk(clean + off, first ? FIRST_CHUNK_BYTES : CHUNK_BYTES);
        if (!n) break;
        esp_http_client_handle_t h = get_client();
        if (!h) break;
        esp_http_client_set_url(h, url);
        esp_http_client_set_method(h, HTTP_METHOD_POST);
        int rc = speak_chunk(h, &s_r, voice, lang, clean + off, n, gen);
        s_last_us = esp_timer_get_time();
        if (rc < 0) break;
        if (rc == 0) {
            // A fresh connection for the retry; the old one may be dead.
            fails++;
            ESP_LOGW(TAG, "retrying chunk (%d/3)", fails);
            drop_client();
            continue;
        }
        first = false;
        off += n;
    }
    free(clean);
    ESP_LOGI(TAG, "request %u done (audio=%d)", (unsigned)gen, (int)s_got);
}

static void worker(void *arg) {
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xSemaphoreTake(s_lock, portMAX_DELAY);
        char *text = s_pending;
        uint32_t gen = s_pending_gen;
        s_pending = NULL;
        xSemaphoreGive(s_lock);
        if (!text) {
            if (s_prewarm) {
                s_prewarm = false;
                warm();
            }
            continue;
        }
        if (gen == s_gen) {
            xStreamBufferReset(s_sb);
            s_done = false;
            s_got = false;
            s_ready_gen = gen;
            run_request(text, gen);
            if (s_gen == gen) s_done = true;
        }
        free(text);
    }
}

static bool ensure_task(void) {
    if (s_task) return true;
    s_sb = xStreamBufferCreateWithCaps(SB_SIZE, 1, MALLOC_CAP_SPIRAM);
    s_lock = xSemaphoreCreateMutex();
    if (!s_sb || !s_lock) return false;
    if (xTaskCreate(worker, "sc_tts", 10 * 1024, NULL, 4, &s_task) != pdPASS) {
        s_task = NULL;
        return false;
    }
    return true;
}

void stackchan_tts_prewarm(void) {
    if (!stackchan_tts_enabled() || !ensure_task()) return;
    s_prewarm = true;
    xTaskNotifyGive(s_task);
}

bool stackchan_tts_begin(const char *text) {
    if (!text || !text[0] || !stackchan_tts_enabled() || !ensure_task()) return false;
    char *copy = heap_caps_malloc(strlen(text) + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!copy) return false;
    strcpy(copy, text);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    free(s_pending);
    s_pending = copy;
    s_pending_gen = ++s_gen;
    xSemaphoreGive(s_lock);
    xTaskNotifyGive(s_task);
    return true;
}

size_t stackchan_tts_read(unsigned char *dst, size_t max, bool *ended, bool *failed) {
    *ended = false;
    *failed = false;
    if (!s_sb || s_ready_gen != s_gen) return 0;
    bool done = s_done;
    size_t n = xStreamBufferReceive(s_sb, dst, max, 0);
    if (!n && done) {
        *ended = true;
        *failed = !s_got;
    }
    return n;
}
