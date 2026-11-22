// stackchan.say: text Muse asked Stack-chan to say, outside any chat turn.
// The command handler (Home Link task) posts it here and replies at once; the
// Muse voice task picks it up when idle and plays it like a reply, through
// stackchan_tts.
#include "stackchan_tts.h"

#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "sc.say";

#define SAY_STALE_US (60LL * 1000000)   // not played within this long (device busy): dropped

static SemaphoreHandle_t s_lock;
static char *volatile s_text;  // pending text, or NULL
static int64_t s_posted_us;
static void (*s_wake)(void);

// Bytes of s (at most max) that end on a whole UTF-8 character.
static size_t whole_utf8(const char *s, size_t n, size_t max) {
    if (n <= max) return n;
    size_t i = max;
    while (i && ((unsigned char)s[i] & 0xC0) == 0x80) i--;   // s[i] must start a character
    return i;
}

// Created by the first post (only the command task posts); take() waits for it.
static bool ensure_lock(void) {
    if (!s_lock) s_lock = xSemaphoreCreateMutex();
    return s_lock != NULL;
}

void stackchan_say_set_wake(void (*wake)(void)) {
    s_wake = wake;
}

bool stackchan_say_post(const char *text, size_t *bytes, bool *truncated) {
    if (!text || !text[0] || !ensure_lock()) return false;
    size_t n = strlen(text);
    size_t keep = whole_utf8(text, n, STACKCHAN_SAY_MAX);
    char *copy = heap_caps_malloc(keep + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!copy) return false;
    memcpy(copy, text, keep);
    copy[keep] = 0;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_text) ESP_LOGI(TAG, "replacing a say not yet played");
    free(s_text);
    s_text = copy;
    s_posted_us = esp_timer_get_time();
    xSemaphoreGive(s_lock);
    if (bytes) *bytes = keep;
    if (truncated) *truncated = keep < n;
    ESP_LOGI(TAG, "queued %u bytes%s", (unsigned)keep, keep < n ? " (truncated)" : "");
    if (s_wake) s_wake();
    return true;
}

bool stackchan_say_waiting(void) {
    return s_text != NULL;   // a peek; stackchan_say_take() decides under the lock
}

char *stackchan_say_take(void) {
    if (!s_lock) return NULL;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    char *t = s_text;
    s_text = NULL;
    bool stale = t && esp_timer_get_time() - s_posted_us > SAY_STALE_US;
    xSemaphoreGive(s_lock);
    if (stale) {
        ESP_LOGW(TAG, "dropping a say queued %d s ago", (int)(SAY_STALE_US / 1000000));
        free(t);
        return NULL;
    }
    return t;
}
