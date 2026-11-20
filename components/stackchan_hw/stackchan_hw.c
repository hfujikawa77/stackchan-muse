#include "stackchan_hw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "stackchan_hw";

// CoreS3 internal I2C bus, created by the SDK's board code (esp-bsp).
extern esp_err_t bsp_i2c_init(void);   // idempotent: ESP_OK if the board code already did it
extern i2c_master_bus_handle_t bsp_i2c_get_handle(void);

// ---- PY32 IO expander -------------------------------------------------------
#define PY32_ADDR         0x6F
#define PY32_I2C_HZ       100000
#define REG_VERSION       0x02
#define REG_GPIO_M_L      0x03   // direction, 1 = output
#define REG_GPIO_O_L      0x05   // output level
#define REG_GPIO_PU_L     0x09   // pull-up
#define REG_GPIO_PD_L     0x0B   // pull-down
#define REG_GPIO_DRV_L    0x13   // drive mode, 1 = open drain
#define REG_LED_CFG       0x24   // bits 0..5 count, bit 6 refresh
#define REG_LED_RAM_START 0x30   // RGB565, little endian, 2 bytes per LED
#define PIN_SERVO_POWER   0      // PY32 pin 0 = VM EN: powers the servos
#define PIN_LED_DATA      13     // PY32 pin 13 = RGB LED data line (push-pull, pulled up)

// ---- Servo bus (SCS0009, 1 Mbps) -------------------------------------------
#define SERVO_UART       UART_NUM_1
#define SERVO_PIN_RX     7
#define SERVO_PIN_TX     6
#define SERVO_BAUD       1000000
#define SCS_ID_X         1
#define SCS_ID_Y         2
#define SCS_INST_PING    0x01
#define SCS_INST_WRITE   0x03
#define SCSCL_TORQUE_ENABLE   40
#define SCSCL_GOAL_POSITION_L 42
#define SCSCL_PRESENT_POSITION_L 56
#define SCS_INST_READ    0x02

// ---- CoreS3 AW9523 (0x58): 5 V out to the Stack-chan body ------------------
// BUS_EN (P0 bit 1) and BOOST_EN (P1 bit 7) feed 5 V to the body, which the LEDs need
// (the servos have their own VM switch). The esp-bsp rewrites P0/P1 from its own cache
// whenever it toggles a rail (the speaker, for one), clearing both bits, so a small task
// puts them back.
#define AW9523_ADDR    0x58
#define AW9523_OUT_P0  0x02
#define AW9523_OUT_P1  0x03
#define AW_BUS_EN      0x02
#define AW_BOOST_EN    0x80
#define BUS_GUARD_MS   300

static i2c_master_dev_handle_t s_py32;
static i2c_master_dev_handle_t s_aw9523;
static SemaphoreHandle_t s_lock;
static bool s_led_ready;
static bool s_servo_ready;
static bool s_uart_ok;          // servo UART is set up; servos may still be booting
static bool s_inited;
static uint8_t s_led_ram[STACKCHAN_LED_COUNT * 2];
static int s_x = STACKCHAN_SERVO_X_CENTER;
static int s_y = STACKCHAN_SERVO_Y_CENTER;

// ---- speech animation (#10) ---------------------------------------------------
// muse_audio_write() reports the level of every 20 ms it plays. Sound that lasts
// VOICE_START_MS counts as speech (so a 90 ms UI chirp does not), and speech ends after
// VOICE_HOLD_MS of quiet. While speaking the LEDs follow the level and the head sways a
// little; afterwards both go back to how they were.
#define ANIM_TICK_MS     20      // 50 Hz, as the official StackChan firmware drives its servos
#define LED_EVERY_TICKS  3       // LEDs at ~16 Hz: the I2C bus is shared with the codec
#define VOICE_LEVEL_MIN  0.08f
#define VOICE_START_MS   300
#define VOICE_HOLD_MS    1500   // quiet this long ends speech; shorter pauses between sentences do not
#define SWAY_DEG         8.0f    // either side of where the head was
#define SWAY_HALF_MS     900     // each swing; the spring is tuned to arrive in about this long
#define SPEAK_R 255
#define SPEAK_G 120
#define SPEAK_B 60

static volatile bool s_voice_seen;
static volatile uint32_t s_voice_last_ms;   // last chunk above VOICE_LEVEL_MIN
static volatile uint32_t s_voice_run_ms;    // start of the current run of sound
static volatile float s_level;
static volatile bool s_anim_on = true;

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static esp_err_t py32_write(uint8_t reg, const uint8_t *data, size_t len) {
    uint8_t buf[1 + sizeof(s_led_ram)];
    if (len > sizeof(s_led_ram)) return ESP_ERR_INVALID_SIZE;
    buf[0] = reg;
    memcpy(buf + 1, data, len);
    return i2c_master_transmit(s_py32, buf, len + 1, 200);
}

static esp_err_t py32_write8(uint8_t reg, uint8_t v) { return py32_write(reg, &v, 1); }

static esp_err_t py32_read8(uint8_t reg, uint8_t *v) {
    return i2c_master_transmit_receive(s_py32, &reg, 1, v, 1, 200);
}

static esp_err_t py32_bit(uint8_t reg_l, uint8_t pin, bool on) {
    uint8_t reg = reg_l + (pin >= 8 ? 1 : 0);
    uint8_t mask = 1u << (pin & 7), v;
    esp_err_t err = py32_read8(reg, &v);
    if (err != ESP_OK) return err;
    v = on ? (v | mask) : (v & ~mask);
    return py32_write8(reg, v);
}

// ---- SCS0009 packets --------------------------------------------------------
static void scs_send(uint8_t id, uint8_t inst, uint8_t addr, const uint8_t *data, uint8_t n, bool has_addr) {
    uint8_t pkt[16];
    uint8_t len = has_addr ? (uint8_t)(n + 3) : 2;
    uint8_t i = 0, sum;
    pkt[i++] = 0xFF;
    pkt[i++] = 0xFF;
    pkt[i++] = id;
    pkt[i++] = len;
    pkt[i++] = inst;
    if (has_addr) {
        pkt[i++] = addr;
        memcpy(pkt + i, data, n);
        i += n;
    }
    sum = 0;
    for (uint8_t k = 2; k < i; k++) sum += pkt[k];
    pkt[i++] = (uint8_t)~sum;
    uart_flush_input(SERVO_UART);   // half-duplex: drop any echo before we send
    uart_write_bytes(SERVO_UART, pkt, i);
    uart_wait_tx_done(SERVO_UART, pdMS_TO_TICKS(20));
}

static bool scs_ping(uint8_t id) {
    scs_send(id, SCS_INST_PING, 0, NULL, 0, false);
    uint8_t buf[16];
    int n = uart_read_bytes(SERVO_UART, buf, sizeof(buf), pdMS_TO_TICKS(30));
    // The line may echo our own ping (FF FF id 02 01 chk) first. A reply is FF FF id 02 <error> chk
    // with error 0 in the normal case, so the echo (0x01 in that slot) is not mistaken for one.
    for (int k = 0; k + 5 < n; k++) {
        if (buf[k] == 0xFF && buf[k + 1] == 0xFF && buf[k + 2] == id && buf[k + 3] == 2 && buf[k + 4] == 0x00) return true;
    }
    return false;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Pings both servos; on success marks them ready. Servos switched on a moment ago can take
// a while to answer, and some boots they miss the first ping, so callers retry.
static void motion_teleport_to_present(void);

static bool servo_probe(void) {
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool x_ok = scs_ping(SCS_ID_X), y_ok = scs_ping(SCS_ID_Y);
    xSemaphoreGive(s_lock);
    if (x_ok && y_ok && !s_servo_ready) {
        s_servo_ready = true;
        motion_teleport_to_present();
    }
    return s_servo_ready;
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---- motion: a spring per axis, stepped at 50 Hz -----------------------------------
// The way the official StackChan firmware moves its servos (stackchan/motion/servo.cpp):
// the target only sets where a mass-spring-damper is pulled, the ESP32 steps the spring
// every 20 ms, and each step goes to the servo as a 20 ms move. The servo's own timed
// moves are never relied on, so there is nothing for it to stop and restart.
typedef struct {
    float pos, vel, target;
    float k, c;        // stiffness and damping (unit mass)
    bool active;
} spring_t;

static spring_t s_spx = {STACKCHAN_SERVO_X_CENTER, 0, STACKCHAN_SERVO_X_CENTER, 170, 26, false};
static spring_t s_spy = {STACKCHAN_SERVO_Y_CENTER, 0, STACKCHAN_SERVO_Y_CENTER, 170, 26, false};
static portMUX_TYPE s_spring_mux = portMUX_INITIALIZER_UNLOCKED;

// Critically damped, tuned so a move settles in about `ms` (to ~2 % at 5.8 / omega).
static void spring_aim(spring_t *sp, float target, uint32_t ms) {
    float w = 5800.0f / (float)(ms ? ms : 1);
    w = clampf(w, 4.0f, 40.0f);
    taskENTER_CRITICAL(&s_spring_mux);
    sp->target = target;
    sp->k = w * w;
    sp->c = 2.0f * w;
    sp->active = true;
    taskEXIT_CRITICAL(&s_spring_mux);
}

static void spring_step(spring_t *sp, float dt) {
    if (!sp->active) return;
    float a = -sp->k * (sp->pos - sp->target) - sp->c * sp->vel;
    sp->vel += a * dt;
    sp->pos += sp->vel * dt;
    if (fabsf(sp->pos - sp->target) < 0.05f && fabsf(sp->vel) < 0.5f) {
        sp->pos = sp->target;
        sp->vel = 0;
        sp->active = false;
    }
}

static void servo_send(float x, float y) {
    // Soft limits apply to targets (stackchan_servo_move, the sway). Here only the servo's own
    // range: a head that starts outside the soft limits is eased back by the spring, not snapped.
    x = clampf(x, 0.0f, 300.0f);
    y = clampf(y, 0.0f, 300.0f);
    uint16_t px = (uint16_t)lroundf(1023.0f - x * 1023.0f / 300.0f);
    uint16_t py = (uint16_t)lroundf(1023.0f - y * 1023.0f / 300.0f);
    const uint16_t ms = ANIM_TICK_MS;
    uint8_t dx[6] = {(uint8_t)(px >> 8), (uint8_t)px, (uint8_t)(ms >> 8), (uint8_t)ms, 0, 0};
    uint8_t dy[6] = {(uint8_t)(py >> 8), (uint8_t)py, (uint8_t)(ms >> 8), (uint8_t)ms, 0, 0};
    xSemaphoreTake(s_lock, portMAX_DELAY);
    scs_send(SCS_ID_X, SCS_INST_WRITE, SCSCL_GOAL_POSITION_L, dx, sizeof(dx), true);
    scs_send(SCS_ID_Y, SCS_INST_WRITE, SCSCL_GOAL_POSITION_L, dy, sizeof(dy), true);
    xSemaphoreGive(s_lock);
}

// Steps both springs and sends the result while either is moving.
static void motion_tick(void) {
    spring_t x, y;
    taskENTER_CRITICAL(&s_spring_mux);
    bool moving = s_spx.active || s_spy.active;
    spring_step(&s_spx, ANIM_TICK_MS / 1000.0f);
    spring_step(&s_spy, ANIM_TICK_MS / 1000.0f);
    x = s_spx;
    y = s_spy;
    taskEXIT_CRITICAL(&s_spring_mux);
    if (moving) servo_send(x.pos, y.pos);
}

// Present position in degrees, or -1. The reply is FF FF id 04 <error> <pos hi> <pos lo> chk;
// our own echoed request has the READ instruction (0x02) where the reply has error 0.
static float scs_read_deg(uint8_t id) {
    uint8_t len = 2;
    scs_send(id, SCS_INST_READ, SCSCL_PRESENT_POSITION_L, &len, 1, true);
    uint8_t buf[24];
    int n = uart_read_bytes(SERVO_UART, buf, sizeof(buf), pdMS_TO_TICKS(30));
    for (int k = 0; k + 6 < n; k++) {
        if (buf[k] == 0xFF && buf[k + 1] == 0xFF && buf[k + 2] == id && buf[k + 3] == 4 && buf[k + 4] == 0x00) {
            int pos = (buf[k + 5] << 8) | buf[k + 6];
            if (pos < 0 || pos > 1023) return -1;
            return (1023 - pos) * 300.0f / 1023.0f;
        }
    }
    return -1;
}

// Start the springs where the head really is, so the first move does not snap.
static void motion_teleport_to_present(void) {
    xSemaphoreTake(s_lock, portMAX_DELAY);
    float x = scs_read_deg(SCS_ID_X), y = scs_read_deg(SCS_ID_Y);
    xSemaphoreGive(s_lock);
    if (x < 0 || y < 0) {
        ESP_LOGW(TAG, "could not read servo positions; assuming centre");
        return;
    }
    taskENTER_CRITICAL(&s_spring_mux);
    s_spx.pos = s_spx.target = x;
    s_spy.pos = s_spy.target = y;
    s_spx.vel = s_spy.vel = 0;
    s_spx.active = s_spy.active = false;
    taskEXIT_CRITICAL(&s_spring_mux);
    s_x = (int)lroundf(x);
    s_y = (int)lroundf(y);
    ESP_LOGI(TAG, "servo position x=%.1f y=%.1f", x, y);
}

// Sets BUS_EN / BOOST_EN if either is clear. Read and write back to back, so a bit the
// BSP changed just before is kept. Returns true if it had to change anything.
static bool bus_5v_assert(void) {
    bool changed = false;
    const uint8_t regs[2] = {AW9523_OUT_P0, AW9523_OUT_P1};
    const uint8_t bits[2] = {AW_BUS_EN, AW_BOOST_EN};
    for (int i = 0; i < 2; i++) {
        uint8_t reg = regs[i], v;
        if (i2c_master_transmit_receive(s_aw9523, &reg, 1, &v, 1, 100) != ESP_OK) continue;
        if ((v & bits[i]) == bits[i]) continue;
        uint8_t w[2] = {reg, (uint8_t)(v | bits[i])};
        if (i2c_master_transmit(s_aw9523, w, 2, 100) == ESP_OK) changed = true;
    }
    return changed;
}

static void led_fill_buffer(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < STACKCHAN_LED_COUNT; i++) stackchan_led_set(i, r, g, b);
}

// One task for the 5 V guard and the speech animation, to keep internal RAM down.
static void hw_task(void *arg) {
    (void)arg;
    uint32_t guard_ms = 0, speak_start = 0, sway_next = 0, probe_ms = 0, led_tick = 0;
    int dir = 1;
    bool speaking = false;
    uint8_t saved_led[sizeof(s_led_ram)];
    int saved_x = STACKCHAN_SERVO_X_CENTER, saved_y = STACKCHAN_SERVO_Y_CENTER;
    float smooth = 0;
    int shown = -1;
    uint32_t prev = now_ms(), max_gap = 0, ticks = 0, late = 0;   // timing stats per speech
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(ANIM_TICK_MS));
        uint32_t now = now_ms();
        uint32_t gap = now - prev;
        prev = now;
        if (s_servo_ready) motion_tick();

        guard_ms += ANIM_TICK_MS;
        if (!s_servo_ready && s_uart_ok && (probe_ms += ANIM_TICK_MS) >= 5000) {
            probe_ms = 0;
            if (servo_probe()) ESP_LOGI(TAG, "servos answered late; servo commands enabled");
        }
        if (s_aw9523 && guard_ms >= BUS_GUARD_MS) {
            guard_ms = 0;
            if (bus_5v_assert() && s_led_ready) {
                vTaskDelay(pdMS_TO_TICKS(20));   // LEDs lose their colour without power: send it again
                stackchan_led_show();
            }
        }

        uint32_t last = s_voice_last_ms, run = s_voice_run_ms;
        // Signed: the audio task can stamp `last` after we read `now`, and an unsigned
        // now - last would then wrap to a huge "quiet" and end the speech mid-sentence.
        bool voice = s_anim_on && s_voice_seen && (int32_t)(now - last) < VOICE_HOLD_MS &&
                     (speaking || (int32_t)(last - run) >= VOICE_START_MS);

        if (voice && !speaking) {
            speaking = true;
            xSemaphoreTake(s_lock, portMAX_DELAY);
            memcpy(saved_led, s_led_ram, sizeof(saved_led));
            xSemaphoreGive(s_lock);
            stackchan_servo_position(&saved_x, &saved_y);
            smooth = 0;
            shown = -1;
            speak_start = now;
            sway_next = now;
            dir = 1;
            led_tick = 0;
            max_gap = 0;
            ticks = 0;
            late = 0;
        } else if (!voice && speaking) {
            speaking = false;
            if (s_led_ready) {
                xSemaphoreTake(s_lock, portMAX_DELAY);
                memcpy(s_led_ram, saved_led, sizeof(saved_led));
                xSemaphoreGive(s_lock);
                stackchan_led_show();
            }
            if (s_servo_ready) stackchan_servo_move(saved_x, saved_y, 500);
            ESP_LOGI(TAG, "speech anim: %.1f s, %u ticks, max gap %u ms, %u ticks over 40 ms",
                     (now - speak_start) / 1000.0f, (unsigned)ticks, (unsigned)max_gap, (unsigned)late);
            continue;
        }
        if (!speaking) continue;
        ticks++;
        if (gap > max_gap) max_gap = gap;
        if (gap > 40) late++;

        if (s_led_ready && ++led_tick >= LED_EVERY_TICKS) {
            led_tick = 0;
            float l = s_level;
            smooth = l > smooth ? 0.5f * smooth + 0.5f * l : 0.75f * smooth + 0.25f * l;   // fast up, slow down
            int br = 30 + (int)(smooth * 225.0f);
            if (shown < 0 || br - shown >= 8 || shown - br >= 8) {
                led_fill_buffer((uint8_t)(SPEAK_R * br / 255), (uint8_t)(SPEAK_G * br / 255), (uint8_t)(SPEAK_B * br / 255));
                stackchan_led_show();
                shown = br;
            }
        }
        if (s_servo_ready && (int32_t)(now - sway_next) >= 0) {
            // Swing to the other side: only the target changes; the spring does the easing.
            spring_aim(&s_spx, saved_x + dir * SWAY_DEG, SWAY_HALF_MS);
            dir = -dir;
            sway_next = now + SWAY_HALF_MS;
        }
    }
}

// ---- public -----------------------------------------------------------------
esp_err_t stackchan_hw_init(void) {
    if (s_inited) return s_led_ready ? ESP_OK : ESP_FAIL;
    s_inited = true;
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    bsp_i2c_init();
    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (!bus) {
        ESP_LOGE(TAG, "no internal I2C bus");
        return ESP_ERR_INVALID_STATE;
    }
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PY32_ADDR,
        .scl_speed_hz = PY32_I2C_HZ,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &cfg, &s_py32);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add PY32: %s", esp_err_to_name(err));
        return err;
    }
    uint8_t ver = 0;
    for (int i = 0; i < 10; i++) {
        if (py32_read8(REG_VERSION, &ver) == ESP_OK && ver != 0 && ver != 0xFF) break;
        ver = 0;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (!ver) {
        ESP_LOGE(TAG, "PY32 not responding");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "PY32 version 0x%02X", ver);

    // 5 V to the body (LEDs), and keep it on
    i2c_device_config_t aw = cfg;
    aw.device_address = AW9523_ADDR;
    if (i2c_master_bus_add_device(bus, &aw, &s_aw9523) == ESP_OK) {
        bus_5v_assert();
        vTaskDelay(pdMS_TO_TICKS(50));
        if (xTaskCreate(hw_task, "sc_hw", 3072, NULL, 5, NULL) != pdPASS) {
            ESP_LOGW(TAG, "hw task not started (5V guard, speech animation)");
        }
    } else {
        ESP_LOGW(TAG, "AW9523 not added; LEDs may stay dark");
    }

    // LEDs: the data pin must be an output, pulled up, push-pull before the LED engine drives it.
    py32_bit(REG_GPIO_M_L, PIN_LED_DATA, true);
    py32_bit(REG_GPIO_PD_L, PIN_LED_DATA, false);
    py32_bit(REG_GPIO_PU_L, PIN_LED_DATA, true);
    py32_bit(REG_GPIO_DRV_L, PIN_LED_DATA, false);
    py32_write8(REG_LED_CFG, STACKCHAN_LED_COUNT & 0x3F);
    memset(s_led_ram, 0, sizeof(s_led_ram));
    s_led_ready = true;
    stackchan_led_show();

    // Servos: switch their power on (PY32 pin 0, output, pulled up, high), then open the UART.
    py32_bit(REG_GPIO_M_L, PIN_SERVO_POWER, true);
    py32_bit(REG_GPIO_PD_L, PIN_SERVO_POWER, false);
    py32_bit(REG_GPIO_PU_L, PIN_SERVO_POWER, true);
    py32_bit(REG_GPIO_O_L, PIN_SERVO_POWER, true);
    vTaskDelay(pdMS_TO_TICKS(200));

    uart_config_t uc = {
        .baud_rate = SERVO_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    if (uart_driver_install(SERVO_UART, 256, 0, 0, NULL, 0) != ESP_OK ||
        uart_param_config(SERVO_UART, &uc) != ESP_OK ||
        uart_set_pin(SERVO_UART, SERVO_PIN_TX, SERVO_PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        ESP_LOGE(TAG, "servo UART setup failed");
        return ESP_OK;   // LEDs still work
    }
    s_uart_ok = true;
    int tries = 0;
    while (!servo_probe() && ++tries < 12) vTaskDelay(pdMS_TO_TICKS(150));   // up to ~2 s
    if (s_servo_ready) ESP_LOGI(TAG, "servos answered (try %d)", tries + 1);
    else ESP_LOGW(TAG, "servos not answering yet; will keep trying every 5 s");
    return ESP_OK;
}

bool stackchan_led_ready(void) { return s_led_ready; }
bool stackchan_servo_ready(void) { return s_servo_ready; }

esp_err_t stackchan_led_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (!s_led_ready) return ESP_ERR_INVALID_STATE;
    if (index >= STACKCHAN_LED_COUNT) return ESP_ERR_INVALID_ARG;
    uint16_t v = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_led_ram[index * 2] = (uint8_t)(v & 0xFF);
    s_led_ram[index * 2 + 1] = (uint8_t)(v >> 8);
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

esp_err_t stackchan_led_show(void) {
    if (!s_led_ready) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    esp_err_t err = py32_write(REG_LED_RAM_START, s_led_ram, sizeof(s_led_ram));
    if (err == ESP_OK) {
        uint8_t cfg = 0;
        err = py32_read8(REG_LED_CFG, &cfg);
        if (err == ESP_OK) err = py32_write8(REG_LED_CFG, cfg | (1 << 6));
    }
    xSemaphoreGive(s_lock);
    return err;
}

esp_err_t stackchan_led_fill(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < STACKCHAN_LED_COUNT; i++) {
        esp_err_t err = stackchan_led_set(i, r, g, b);
        if (err != ESP_OK) return err;
    }
    return stackchan_led_show();
}

esp_err_t stackchan_servo_move(int x_deg, int y_deg, uint32_t ms) {
    if (!s_servo_ready) return ESP_ERR_INVALID_STATE;
    x_deg = clampi(x_deg, STACKCHAN_SERVO_X_MIN, STACKCHAN_SERVO_X_MAX);
    y_deg = clampi(y_deg, STACKCHAN_SERVO_Y_MIN, STACKCHAN_SERVO_Y_MAX);
    if (ms < STACKCHAN_SERVO_MIN_MOVE_MS) ms = STACKCHAN_SERVO_MIN_MOVE_MS;
    // The 50 Hz motion task takes it from here.
    spring_aim(&s_spx, (float)x_deg, ms);
    spring_aim(&s_spy, (float)y_deg, ms);
    s_x = x_deg;
    s_y = y_deg;
    return ESP_OK;
}

esp_err_t stackchan_servo_center(uint32_t ms) {
    return stackchan_servo_move(STACKCHAN_SERVO_X_CENTER, STACKCHAN_SERVO_Y_CENTER, ms);
}

esp_err_t stackchan_servo_torque(bool on) {
    if (!s_servo_ready) return ESP_ERR_INVALID_STATE;
    uint8_t v = on ? 1 : 0;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    scs_send(SCS_ID_X, SCS_INST_WRITE, SCSCL_TORQUE_ENABLE, &v, 1, true);
    scs_send(SCS_ID_Y, SCS_INST_WRITE, SCSCL_TORQUE_ENABLE, &v, 1, true);
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

void stackchan_servo_position(int *x_deg, int *y_deg) {
    if (x_deg) *x_deg = s_x;
    if (y_deg) *y_deg = s_y;
}

bool stackchan_hw_console_command(const char *line) {
    int a, b, c, d = 600;
    if (!strncmp(line, "i2cr=", 5) || !strncmp(line, "i2cw=", 5)) {
        // debug: >i2cr=ADDR,REG[,LEN]   >i2cw=ADDR,REG,VAL   (writes only to the AW9523 at 0x58)
        bool wr = line[3] == 'w';
        int len = 1;
        int n = sscanf(line + 5, "%i,%i,%i", &a, &b, &c);
        i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
        if (!bus || n < 2 || a < 0x08 || a > 0x77 || b < 0 || b > 0xFF || (wr && (n < 3 || a != 0x58))) {
            printf("@i2c usage: >i2cr=ADDR,REG[,LEN]  >i2cw=0x58,REG,VAL\n");
        } else {
            i2c_device_config_t dc = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = (uint16_t)a, .scl_speed_hz = 100000};
            i2c_master_dev_handle_t dev;
            esp_err_t err = i2c_master_bus_add_device(bus, &dc, &dev);
            uint8_t buf[32] = {0};
            if (err == ESP_OK) {
                uint8_t reg = (uint8_t)b;
                if (wr) {
                    uint8_t w[2] = {reg, (uint8_t)c};
                    err = i2c_master_transmit(dev, w, 2, 200);
                } else {
                    if (n >= 3) len = c < 1 ? 1 : (c > 32 ? 32 : c);
                    err = i2c_master_transmit_receive(dev, &reg, 1, buf, len, 200);
                }
                i2c_master_bus_rm_device(dev);
            }
            printf("@i2c%c 0x%02X 0x%02X %s:", wr ? 'w' : 'r', a, b, err == ESP_OK ? "ok" : esp_err_to_name(err));
            if (!wr) for (int i = 0; i < len; i++) printf(" %02X", buf[i]);
            printf("\n");
        }
    } else if (!strncmp(line, "py32r=", 6)) {   // debug: >py32r=REG[,LEN] prints PY32 registers
        int len = 1;
        if (!s_led_ready || sscanf(line + 6, "%i,%d", &a, &len) < 1 || a < 0 || a > 0xFF) {
            printf("@py32r usage: >py32r=REG[,LEN]\n");
        } else {
            if (len < 1) len = 1;
            if (len > 32) len = 32;
            uint8_t buf[32] = {0};
            xSemaphoreTake(s_lock, portMAX_DELAY);
            uint8_t reg = (uint8_t)a;
            esp_err_t err = i2c_master_transmit_receive(s_py32, &reg, 1, buf, len, 200);
            xSemaphoreGive(s_lock);
            printf("@py32r 0x%02X %s:", a, err == ESP_OK ? "ok" : esp_err_to_name(err));
            for (int i = 0; i < len; i++) printf(" %02X", buf[i]);
            printf("\n");
        }
    } else if (!strncmp(line, "anim=", 5)) {
        stackchan_hw_set_speech_anim(line[5] == '1');
        printf("@anim %s\n", line[5] == '1' ? "on" : "off");
    } else if (!strncmp(line, "led=", 4)) {
        if (sscanf(line + 4, "%d,%d,%d", &a, &b, &c) != 3) {
            printf("@led usage: >led=R,G,B\n");
        } else {
            esp_err_t err = stackchan_led_fill((uint8_t)clampi(a, 0, 255), (uint8_t)clampi(b, 0, 255), (uint8_t)clampi(c, 0, 255));
            printf("@led %s\n", err == ESP_OK ? "ok" : esp_err_to_name(err));
        }
    } else if (!strncmp(line, "servo=", 6)) {
        const char *v = line + 6;
        esp_err_t err;
        if (!strcmp(v, "center")) err = stackchan_servo_center(800);
        else if (!strcmp(v, "off")) err = stackchan_servo_torque(false);
        else if (!strcmp(v, "on")) err = stackchan_servo_torque(true);
        else if (sscanf(v, "%d,%d,%d", &a, &b, &d) >= 2) err = stackchan_servo_move(a, b, (uint32_t)(d < 0 ? 0 : d));
        else {
            printf("@servo usage: >servo=X,Y[,MS] | center | off | on\n");
            fflush(stdout);
            return true;
        }
        int x, y;
        stackchan_servo_position(&x, &y);
        printf("@servo %s x=%d y=%d\n", err == ESP_OK ? "ok" : esp_err_to_name(err), x, y);
    } else {
        return false;
    }
    fflush(stdout);
    return true;
}

void stackchan_hw_audio_level(float level) {
    s_level = level;
    if (level < VOICE_LEVEL_MIN) return;
    uint32_t now = now_ms();
    if (!s_voice_seen || (int32_t)(now - s_voice_last_ms) > VOICE_HOLD_MS) s_voice_run_ms = now;
    s_voice_last_ms = now;
    s_voice_seen = true;
}

void stackchan_hw_set_speech_anim(bool on) { s_anim_on = on; }
