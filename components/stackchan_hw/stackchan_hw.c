#include "stackchan_hw.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "esp_random.h"
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
#define SCSCL_MIN_ANGLE_LIMIT_L  9    // min/max angle limits 0/0 = wheel (continuous) mode
#define SCSCL_GOAL_TIME_L        44   // in wheel mode: speed, bit 10 = direction
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
// Sway defaults; >sway=DEG,HALF_MS changes them at run time for tuning.
#define SWAY_DEG_DEFAULT      15.0f   // either side of where the head was
#define SWAY_HALF_MS_DEFAULT  700     // each swing; the spring is tuned to arrive in about this long
#define SWAY_UP_DEFAULT       8.0f    // look up this much while speaking
#define SPEAK_R 255
#define SPEAK_G 120
#define SPEAK_B 60

static volatile bool s_voice_seen;
static volatile uint32_t s_voice_last_ms;   // last chunk above VOICE_LEVEL_MIN
static volatile uint32_t s_voice_run_ms;    // start of the current run of sound
static volatile float s_level;
static volatile bool s_anim_on = true;
static volatile float s_sway_deg = SWAY_DEG_DEFAULT;
static volatile uint32_t s_sway_half_ms = SWAY_HALF_MS_DEFAULT;
static volatile float s_sway_up = SWAY_UP_DEFAULT;
static volatile uint32_t s_fake_voice_until;   // >swaytest: animate as if speaking until then
static volatile bool s_fake_voice;

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
static bool motion_teleport_to_present(void);

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

// ---- centre calibration ------------------------------------------------------------
// Each head sits a little differently on its servos. >servo=zero takes where the head is
// now as the centre (X 150, Y 90) and keeps the difference in NVS, as the official firmware
// keeps its zero positions. The rest of the code works in calibrated angles; only
// servo_send() and scs_read_deg() see the servo's own.
static float s_off_x, s_off_y;
static volatile bool s_spin_x;   // X is in wheel mode: no position writes (they would set its speed)

static void calib_load(void) {
    nvs_handle_t h;
    if (nvs_open("stackchan", NVS_READONLY, &h) != ESP_OK) return;
    int32_t zx = 0, zy = 0;
    if (nvs_get_i32(h, "zero_x", &zx) == ESP_OK && nvs_get_i32(h, "zero_y", &zy) == ESP_OK) {
        s_off_x = zx / 10.0f;
        s_off_y = zy / 10.0f;
        ESP_LOGI(TAG, "centre calibration: x%+.1f y%+.1f deg", s_off_x, s_off_y);
    }
    nvs_close(h);
}

static esp_err_t calib_save(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open("stackchan", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_i32(h, "zero_x", (int32_t)lroundf(s_off_x * 10.0f));
    if (err == ESP_OK) err = nvs_set_i32(h, "zero_y", (int32_t)lroundf(s_off_y * 10.0f));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

static void servo_send(float x, float y) {
    x += s_off_x;   // calibrated -> the servo's own angle
    y += s_off_y;
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
    if (!s_spin_x) scs_send(SCS_ID_X, SCS_INST_WRITE, SCSCL_GOAL_POSITION_L, dx, sizeof(dx), true);
    scs_send(SCS_ID_Y, SCS_INST_WRITE, SCSCL_GOAL_POSITION_L, dy, sizeof(dy), true);
    xSemaphoreGive(s_lock);
}

// X in wheel mode turning at `speed` (0..1000, negative the other way), or back to positions.
// As HermesLLM's turnXBegin/turnXEnd and the official firmware's rotate(): the angle limits
// 0/0 make an SCS0009 turn continuously, and its goal-time register then sets the speed.
static void spin_x(bool on, int speed) {
    uint8_t lim[4] = {0, 0, 0, 0};
    if (!on) {
        lim[1] = 0x14;   // limits 20..1023: position mode again
        lim[2] = 0x03;
        lim[3] = 0xFF;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (on) s_spin_x = true;
    scs_send(SCS_ID_X, SCS_INST_WRITE, SCSCL_MIN_ANGLE_LIMIT_L, lim, sizeof(lim), true);
    int v = speed < 0 ? ((-speed) | (1 << 10)) : speed;
    if (!on) v = 0;
    uint8_t sp[2] = {(uint8_t)(v >> 8), (uint8_t)v};
    scs_send(SCS_ID_X, SCS_INST_WRITE, SCSCL_GOAL_TIME_L, sp, sizeof(sp), true);
    if (!on) s_spin_x = false;
    xSemaphoreGive(s_lock);
}

static volatile int s_resend_ticks;   // keep sending the spring position this many ticks more

// Steps both springs and sends the result while either is moving (or a resend is pending).
static void motion_tick(void) {
    spring_t x, y;
    taskENTER_CRITICAL(&s_spring_mux);
    bool moving = s_spx.active || s_spy.active;
    spring_step(&s_spx, ANIM_TICK_MS / 1000.0f);
    spring_step(&s_spy, ANIM_TICK_MS / 1000.0f);
    x = s_spx;
    y = s_spy;
    taskEXIT_CRITICAL(&s_spring_mux);
    if (s_resend_ticks > 0) {
        s_resend_ticks--;
        moving = true;
    }
    if (moving) servo_send(x.pos, y.pos);
}

// Present position in calibrated degrees, or -1. The reply is FF FF id 04 <error> <pos hi> <pos lo> chk;
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
            return (1023 - pos) * 300.0f / 1023.0f - (id == SCS_ID_X ? s_off_x : s_off_y);   // calibrated
        }
    }
    return -1;
}

// Start the springs where the head really is, so the first move does not snap.
// False if the servos could not be read (the springs keep what they had).
static bool motion_teleport_to_present(void) {
    xSemaphoreTake(s_lock, portMAX_DELAY);
    float x = scs_read_deg(SCS_ID_X), y = scs_read_deg(SCS_ID_Y);
    xSemaphoreGive(s_lock);
    if (x < 0 || y < 0) {
        ESP_LOGW(TAG, "could not read servo positions; keeping the last known");
        return false;
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
    return true;
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

// ---- head touch (Si12T, I2C 0x68) and the pet reaction ---------------------------
// As the official StackChan firmware does it (hal_head_touch.cpp, modifiers/head_pet.h):
// three pads give 0..3 each; their weighted centre is a position from -100 (front) to
// +100 (back). Touch, then a move of the position past SWIPE_THRESHOLD, is a stroke. A
// stroke makes the robot happy (pink LEDs) and it looks up or tilts its head; RESTORE_MS
// after the hand leaves, LEDs and head go back.
#define SI12T_ADDR          0x68
#define SI12T_SENS1         0x02   // .. 0x06
#define SI12T_CTRL1         0x08
#define SI12T_CTRL2         0x09
#define SI12T_REF_RST1      0x0A
#define SI12T_REF_RST2      0x0B
#define SI12T_CH_HOLD1      0x0C
#define SI12T_CH_HOLD2      0x0D
#define SI12T_CAL_HOLD1     0x0E
#define SI12T_CAL_HOLD2     0x0F
#define SI12T_OUTPUT1       0x10
#define SI12T_SENS_LOW_3    0x33   // official: SI12T_TYPE_LOW, SI12T_SENSITIVITY_LEVEL_3
#define TOUCH_EVERY_TICKS   3      // ~60 ms (official 50 ms)
#define SWIPE_THRESHOLD     40
#define PET_RESTORE_MS      3000
#define PET_R 255
#define PET_G 60
#define PET_B 140

static i2c_master_dev_handle_t s_si12t;
static bool s_touch_ready;
static volatile bool s_pet_on = true;
static volatile uint8_t s_touch_raw;   // last OUTPUT1, for >touch
static void (*volatile s_pet_cb)(void);   // the face's happy reaction (muse_state_make_happy)
#define PET_FACE_EVERY_MS   1000   // the face stays happy 1.6 s per call: renew it while happy
// Joy: +1 a stroke, cooling 0.5 a second. 1-2: look up; 3-5: wag the head; 6+: a full turn.
#define JOY_MAX             10.0f
#define JOY_COOL_PER_S      0.5f
#define JOY_WAG             2.5f    // 3 strokes, allowing for cooling between them
#define JOY_SPIN            5.5f    // 6 strokes
#define WAG_MS              1200    // a wag lasts this long
#define WAG_STEP_MS         220
#define WAG_DEG             8
#define WAG_NOD_DEG         10      // a vertical wag (nod) looks up this much
#define WAG_NOD_ONE_IN      4       // about one wag in four is a nod
#define SPIN_SPEED          500     // as HermesLLM: 500 for 2 s is about one turn
#define SPIN_MS             2000
static volatile int s_fake_strokes;   // >stroke=N: as if stroked N times

typedef enum { G_NONE, G_PRESS, G_RELEASE, G_SWIPE_FWD, G_SWIPE_BACK } gesture_t;

static esp_err_t si12t_w(uint8_t reg, uint8_t v) {
    uint8_t b[2] = {reg, v};
    return i2c_master_transmit(s_si12t, b, 2, 100);
}

static esp_err_t si12t_setup(i2c_master_bus_handle_t bus) {
    i2c_device_config_t dc = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = SI12T_ADDR, .scl_speed_hz = 100000};
    esp_err_t err = i2c_master_bus_add_device(bus, &dc, &s_si12t);
    if (err != ESP_OK) return err;
    // enable channels and reference calibration
    const uint8_t zero_regs[] = {SI12T_REF_RST1, SI12T_REF_RST2, SI12T_CH_HOLD1, SI12T_CH_HOLD2, SI12T_CAL_HOLD1, SI12T_CAL_HOLD2};
    for (size_t i = 0; i < sizeof(zero_regs) && err == ESP_OK; i++) err = si12t_w(zero_regs[i], 0x00);
    // soft reset, then auto mode, FTC=01, interrupt middle/high, response 4
    if (err == ESP_OK) err = si12t_w(SI12T_CTRL2, 0x0F);
    if (err == ESP_OK) err = si12t_w(SI12T_CTRL2, 0x07);
    if (err == ESP_OK) err = si12t_w(SI12T_CTRL1, 0x22);
    for (uint8_t r = SI12T_SENS1; r < SI12T_SENS1 + 5 && err == ESP_OK; r++) err = si12t_w(r, SI12T_SENS_LOW_3);
    return err;
}

static bool si12t_read(uint8_t level[3]) {
    uint8_t reg = SI12T_OUTPUT1, v = 0;
    if (i2c_master_transmit_receive(s_si12t, &reg, 1, &v, 1, 100) != ESP_OK) return false;
    s_touch_raw = v;
    for (int i = 0; i < 3; i++) level[i] = (v >> (2 * i)) & 0x03;
    return true;
}

// The official recogniser: IDLE -> TOUCHED (press) -> SWIPING (stroke) -> IDLE (release).
static gesture_t touch_update(const uint8_t level[3]) {
    static enum { T_IDLE, T_TOUCHED, T_SWIPING } st = T_IDLE;
    static int start_pos;
    int total = level[0] + level[1] + level[2];
    bool touched = level[0] >= 1 || level[1] >= 1 || level[2] >= 1;
    int pos = total ? (level[0] * -100 + level[2] * 100) / total : 0;
    switch (st) {
    case T_IDLE:
        if (touched) {
            st = T_TOUCHED;
            start_pos = pos;
            return G_PRESS;
        }
        break;
    case T_TOUCHED:
        if (!touched) {
            st = T_IDLE;
            return G_RELEASE;
        }
        if (pos - start_pos > SWIPE_THRESHOLD) {
            st = T_SWIPING;
            return G_SWIPE_FWD;
        }
        if (pos - start_pos < -SWIPE_THRESHOLD) {
            st = T_SWIPING;
            return G_SWIPE_BACK;
        }
        break;
    case T_SWIPING:
        if (!touched) {
            st = T_IDLE;
            return G_RELEASE;
        }
        break;
    }
    return G_NONE;
}

static int rand_between(int lo, int hi) { return lo + (int)(esp_random() % (uint32_t)(hi - lo + 1)); }

// ---- LEDs: layers and effects (#11) ---------------------------------------------------
// The LEDs show the top layer that is active: speech > pet > listening/thinking > an effect
// > the base colours (stackchan_led_set/fill). Each layer is drawn into s_led_ram (the frame)
// by hw_task; when a layer ends, the one below simply shows again, so nothing has to save
// and restore colours. Effects follow the HermesLLM patterns.
static uint8_t s_base[STACKCHAN_LED_COUNT * 2];   // base colours, RGB565 as in the LED RAM
static volatile bool s_base_dirty;
static volatile bool s_task_running;
static volatile int s_ui_mode;                    // muse_mode_t, from stackchan_hw_ui_mode()
static volatile bool s_ui_fx_on = true;

typedef struct {
    int fx;
    uint8_t r, g, b, r2, g2, b2;
    int speed;   // 1..10
} fx_t;
static fx_t s_fx;                                 // the effect layer; fx == STACKCHAN_FX_NONE is off
static volatile bool s_fx_dirty;
static portMUX_TYPE s_fx_mux = portMUX_INITIALIZER_UNLOCKED;

static const char *const k_fx_names[] = {"off", "solid", "blink", "breath", "chase", "rainbow", "wave", "alternate"};

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void frame_set(int i, uint8_t r, uint8_t g, uint8_t b) {
    uint16_t v = rgb565(r, g, b);
    s_led_ram[i * 2] = (uint8_t)(v & 0xFF);
    s_led_ram[i * 2 + 1] = (uint8_t)(v >> 8);
}

static void frame_fill(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < STACKCHAN_LED_COUNT; i++) frame_set(i, r, g, b);
}

static uint8_t scale8(uint8_t v, float k) { return (uint8_t)(v * k); }

// Sends s_led_ram to the LEDs.
static esp_err_t frame_send(void) {
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

// Full-saturation hue (degrees) to RGB.
static void hue_rgb(int hue, uint8_t *r, uint8_t *g, uint8_t *b) {
    hue %= 360;
    if (hue < 0) hue += 360;
    int x = (hue % 60) * 255 / 60;
    switch (hue / 60) {
    case 0: *r = 255; *g = x; *b = 0; break;
    case 1: *r = 255 - x; *g = 255; *b = 0; break;
    case 2: *r = 0; *g = 255; *b = x; break;
    case 3: *r = 0; *g = 255 - x; *b = 255; break;
    case 4: *r = x; *g = 0; *b = 255; break;
    default: *r = 255; *g = 0; *b = 255 - x; break;
    }
}

// Draws one frame of an effect at time t (ms) into s_led_ram. Speed 1 (slow) .. 10 (fast).
static void render_fx(const fx_t *f, uint32_t t) {
    int sp = f->speed < 1 ? 1 : (f->speed > 10 ? 10 : f->speed);
    switch (f->fx) {
    case STACKCHAN_FX_SOLID:
        frame_fill(f->r, f->g, f->b);
        break;
    case STACKCHAN_FX_BLINK: {   // also the strobe: speed 5 is 2.5 flashes a second
        uint32_t period = 2000 / sp, on = period / 4 < 40 ? 40 : period / 4;
        bool lit = (t % period) < on;
        frame_fill(lit ? f->r : 0, lit ? f->g : 0, lit ? f->b : 0);
        break;
    }
    case STACKCHAN_FX_BREATH: {
        uint32_t period = 1000 + 6000 / sp;   // speed 5: 2.2 s
        float k = (1.0f - cosf(2.0f * (float)M_PI * (float)(t % period) / period)) / 2.0f;
        k = 0.04f + 0.96f * k;
        frame_fill(scale8(f->r, k), scale8(f->g, k), scale8(f->b, k));
        break;
    }
    case STACKCHAN_FX_CHASE: {   // three dots a third of the ring apart, with a short tail
        uint32_t step = 600 / sp;            // speed 5: 120 ms per LED
        int pos = (int)((t / step) % STACKCHAN_LED_COUNT);
        frame_fill(0, 0, 0);
        for (int k = 0; k < 3; k++) {
            int i = (pos + k * 4) % STACKCHAN_LED_COUNT;
            int tail = (i + STACKCHAN_LED_COUNT - 1) % STACKCHAN_LED_COUNT;
            frame_set(tail, scale8(f->r, 0.25f), scale8(f->g, 0.25f), scale8(f->b, 0.25f));
            frame_set(i, f->r, f->g, f->b);
        }
        break;
    }
    case STACKCHAN_FX_RAINBOW: {
        int base = (int)((t * (uint32_t)sp * 36u / 1000u) % 360u);   // speed 5: a turn in 2 s
        for (int i = 0; i < STACKCHAN_LED_COUNT; i++) {
            uint8_t r, g, b;
            hue_rgb(base + i * 360 / STACKCHAN_LED_COUNT, &r, &g, &b);
            frame_set(i, r, g, b);
        }
        break;
    }
    case STACKCHAN_FX_WAVE: {
        uint32_t period = 6000 / sp;         // speed 5: 1.2 s for the wave to go round
        float ph = (float)(t % period) / period;
        for (int i = 0; i < STACKCHAN_LED_COUNT; i++) {
            float k = (1.0f + sinf(2.0f * (float)M_PI * ((float)i / STACKCHAN_LED_COUNT - ph))) / 2.0f;
            k = 0.05f + 0.95f * k;
            frame_set(i, scale8(f->r, k), scale8(f->g, k), scale8(f->b, k));
        }
        break;
    }
    case STACKCHAN_FX_ALTERNATE: {
        uint32_t interval = 1500 / sp;       // speed 5: swap every 300 ms
        bool swap = (t / interval) & 1;
        for (int i = 0; i < STACKCHAN_LED_COUNT; i++) {
            bool first = ((i & 1) == 0) != swap;
            frame_set(i, first ? f->r : f->r2, first ? f->g : f->g2, first ? f->b : f->b2);
        }
        break;
    }
    default:
        frame_fill(0, 0, 0);
        break;
    }
}

static void fx_get(fx_t *f) {
    taskENTER_CRITICAL(&s_fx_mux);
    *f = s_fx;
    taskEXIT_CRITICAL(&s_fx_mux);
}

static void fx_put(const fx_t *f) {
    taskENTER_CRITICAL(&s_fx_mux);
    s_fx = *f;
    taskEXIT_CRITICAL(&s_fx_mux);
    s_fx_dirty = true;
}

// Without a task to draw them, base colours go straight out.
static void base_changed(void) {
    s_base_dirty = true;
    if (!s_task_running) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        memcpy(s_led_ram, s_base, sizeof(s_led_ram));
        xSemaphoreGive(s_lock);
        frame_send();
    }
}

enum { L_BASE, L_FX, L_UI, L_PET, L_SPEAK };

// One task for the 5 V guard, the head touch, the speech animation and the LED layers,
// to keep internal RAM down.
static void hw_task(void *arg) {
    (void)arg;
    s_task_running = true;
    uint32_t guard_ms = 0, speak_start = 0, sway_next = 0, probe_ms = 0, led_tick = 0;
    int dir = 1;
    // pet reaction state
    uint32_t touch_tick = 0, pet_restore_at = 0, pet_face_at = 0;
    float joy = 0;
    bool spinning = false, spun = false;
    uint32_t spin_end = 0, wag_until = 0, wag_next = 0;
    int wag_dir = 1;
    bool wag_vertical = false;   // now and then the wag is a nod
    bool happy = false, wait_restore = false;
    int pet_x = STACKCHAN_SERVO_X_CENTER, pet_y = STACKCHAN_SERVO_Y_CENTER;
    bool speaking = false;
    int saved_x = STACKCHAN_SERVO_X_CENTER, saved_y = STACKCHAN_SERVO_Y_CENTER;
    float smooth = 0;
    int shown = -1;
    int layer = -1;          // LED layer drawn last
    bool led_force = true;   // redraw on the next LED frame
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
                led_force = true;
            }
        }

        if (s_touch_ready && ++touch_tick >= TOUCH_EVERY_TICKS) {
            touch_tick = 0;
            uint8_t level[3];
            gesture_t g = si12t_read(level) ? touch_update(level) : G_NONE;
            bool fake_stroke = false;
            if (g == G_NONE && s_fake_strokes > 0) {   // >stroke=N, one per poll
                fake_stroke = true;
                s_fake_strokes--;
                g = G_SWIPE_FWD;
                if (s_fake_strokes == 0) {   // and then as if the hand left
                    pet_restore_at = now + PET_RESTORE_MS;
                }
            }
            if (g == G_SWIPE_FWD || g == G_SWIPE_BACK) {
                ESP_LOGI(TAG, "head: stroke %s", g == G_SWIPE_FWD ? "forward" : "backward");
                if (s_pet_on && !speaking) {
                    if (!happy) {   // remember where the head was, to go back to
                        happy = true;
                        stackchan_servo_position(&pet_x, &pet_y);
                    }
                    wait_restore = false;   // still being stroked
                    if (!fake_stroke) pet_restore_at = 0;   // a hand is on the head: no going back yet
                    if (s_pet_cb) s_pet_cb();   // hop, blush and hearts on the face
                    pet_face_at = now + PET_FACE_EVERY_MS;
                    joy = joy + 1.0f > JOY_MAX ? JOY_MAX : joy + 1.0f;
                    ESP_LOGI(TAG, "head: joy %.1f", joy);
                    bool moving;
                    taskENTER_CRITICAL(&s_spring_mux);
                    moving = s_spx.active || s_spy.active;
                    taskEXIT_CRITICAL(&s_spring_mux);
                    if (s_servo_ready && joy >= JOY_SPIN && !spun && !spinning) {
                        // Overjoyed: a full turn of the head, once per happy spell.
                        spun = true;
                        spinning = true;
                        spin_end = now + SPIN_MS;
                        wag_until = 0;
                        spin_x(true, SPIN_SPEED);
                        ESP_LOGI(TAG, "head: overjoyed, spinning");
                    } else if (s_servo_ready && joy >= JOY_WAG && !spinning) {
                        // Happy: wag the head side to side, like a tail; now and then a nod instead.
                        if ((int32_t)(now - wag_until) >= 0) {   // a new wag: pick its direction
                            wag_next = now;
                            wag_dir = 1;
                            wag_vertical = rand_between(1, WAG_NOD_ONE_IN) == 1;
                        }
                        wag_until = now + WAG_MS;
                    } else if (s_servo_ready && !moving && !spinning) {
                        // Y smaller = looking up. Soft limits keep it within 20 deg up.
                        int tx = pet_x, ty = pet_y;
                        switch (rand_between(0, 2)) {
                        case 0:   // look up
                            ty -= rand_between(15, 25);
                            tx += rand_between(-5, 5);
                            break;
                        case 1:   // tilt to one side
                            ty += rand_between(0, 5);
                            tx += rand_between(0, 1) ? 15 : -15;
                            break;
                        default:  // big happy look up
                            ty -= rand_between(25, 40);
                            break;
                        }
                        uint32_t ms = (uint32_t)rand_between(400, 600);
                        spring_aim(&s_spx, clampf((float)tx, STACKCHAN_SERVO_X_MIN, STACKCHAN_SERVO_X_MAX), ms);
                        spring_aim(&s_spy, clampf((float)ty, STACKCHAN_SERVO_Y_MIN, STACKCHAN_SERVO_Y_MAX), ms);
                    }
                }
            } else if (g == G_RELEASE) {
                ESP_LOGI(TAG, "head: release");
                if (happy) {
                    wait_restore = true;
                    pet_restore_at = now + PET_RESTORE_MS;
                }
            } else if (g == G_PRESS) {
                ESP_LOGI(TAG, "head: touch");
            }
        }
        joy -= JOY_COOL_PER_S * ANIM_TICK_MS / 1000.0f;
        if (joy < 0) joy = 0;
        if (spinning && (int32_t)(now - spin_end) >= 0) {
            spin_x(false, 0);
            spinning = false;
            // The angle limits are in the servo's EEPROM: let the switch back land before
            // talking to it. The turn rarely ends exactly round, and the last 60 deg of a turn
            // are outside what the servo can measure, so the read may fail; and a single move
            // sent too early can be lost, which left the head facing sideways. So: wait, read
            // with retries, and keep sending the way home for a second.
            vTaskDelay(pdMS_TO_TICKS(120));
            bool found = false;
            for (int i = 0; i < 3 && !found; i++) {
                found = motion_teleport_to_present();   // springs from where the turn left the head
                if (!found) vTaskDelay(pdMS_TO_TICKS(50));
            }
            s_resend_ticks = 1000 / ANIM_TICK_MS;
            spring_aim(&s_spx, (float)pet_x, 800);
            ESP_LOGI(TAG, "head: spin done");
        }
        if (!spinning && happy && (int32_t)(now - wag_until) < 0 && (int32_t)(now - wag_next) >= 0) {
            if (wag_vertical) {   // a nod: up and back (the resting pose is the lowest it goes)
                float y = wag_dir > 0 ? (float)(pet_y - WAG_NOD_DEG) : (float)pet_y;
                spring_aim(&s_spy, clampf(y, STACKCHAN_SERVO_Y_MIN, STACKCHAN_SERVO_Y_MAX), WAG_STEP_MS);
            } else {
                spring_aim(&s_spx, clampf((float)(pet_x + wag_dir * WAG_DEG), STACKCHAN_SERVO_X_MIN, STACKCHAN_SERVO_X_MAX),
                           WAG_STEP_MS);
            }
            wag_dir = -wag_dir;
            wag_next = now + WAG_STEP_MS;
            if ((int32_t)(wag_until - wag_next) <= 0) {   // last step: back to centre
                if (wag_vertical) spring_aim(&s_spy, (float)pet_y, WAG_STEP_MS);
                else spring_aim(&s_spx, (float)pet_x, WAG_STEP_MS);
            }
        }
        if (spinning && pet_restore_at != 0) pet_restore_at = now + PET_RESTORE_MS;   // go back only after the turn
        if (happy && s_pet_cb && (int32_t)(now - pet_face_at) >= 0) {
            s_pet_cb();
            pet_face_at = now + PET_FACE_EVERY_MS;
        }
        if (happy && !wait_restore && s_fake_strokes == 0 && (int32_t)(now - pet_restore_at) >= 0 &&
            pet_restore_at != 0) {
            wait_restore = true;   // fake strokes: no real release comes
        }
        if (happy && wait_restore && !spinning && (int32_t)(now - pet_restore_at) >= 0) {
            happy = false;
            wait_restore = false;
            spun = false;
            wag_until = 0;
            pet_restore_at = 0;
            if (s_servo_ready) {
                spring_aim(&s_spx, (float)pet_x, 900);
                spring_aim(&s_spy, (float)pet_y, 900);
            }
        }

        uint32_t last = s_voice_last_ms, run = s_voice_run_ms;
        // Signed: the audio task can stamp `last` after we read `now`, and an unsigned
        // now - last would then wrap to a huge "quiet" and end the speech mid-sentence.
        bool voice = s_anim_on && s_voice_seen && (int32_t)(now - last) < VOICE_HOLD_MS &&
                     (speaking || (int32_t)(last - run) >= VOICE_START_MS);
        if (s_fake_voice) {
            if ((int32_t)(now - s_fake_voice_until) < 0) voice = true;
            else s_fake_voice = false;
        }

        if (voice && !speaking) {
            speaking = true;
            stackchan_servo_position(&saved_x, &saved_y);
            smooth = 0;
            speak_start = now;
            sway_next = now;
            dir = 1;
            max_gap = 0;
            ticks = 0;
            late = 0;
        } else if (!voice && speaking) {
            speaking = false;
            if (s_servo_ready) stackchan_servo_move(saved_x, saved_y, 500);
            ESP_LOGI(TAG, "speech anim: %.1f s, %u ticks, max gap %u ms, %u ticks over 40 ms",
                     (now - speak_start) / 1000.0f, (unsigned)ticks, (unsigned)max_gap, (unsigned)late);
        }
        if (speaking) {
            ticks++;
            if (gap > max_gap) max_gap = gap;
            if (gap > 40) late++;
            if (s_servo_ready && (int32_t)(now - sway_next) >= 0) {
                // Swing to the other side: only the target changes; the spring does the easing.
                uint32_t half = s_sway_half_ms;
                spring_aim(&s_spx, saved_x + dir * s_sway_deg, half);
                // Look up a little while talking (Y smaller = up); back down when the speech ends.
                spring_aim(&s_spy, clampf(saved_y - s_sway_up, STACKCHAN_SERVO_Y_MIN, STACKCHAN_SERVO_Y_MAX), half);
                dir = -dir;
                sway_next = now + half;
            }
        }

        // ---- LEDs: draw the top active layer ----
        if (!s_led_ready || ++led_tick < LED_EVERY_TICKS) continue;
        led_tick = 0;
        int mode = s_ui_mode;
        bool ui = s_ui_fx_on && (mode == STACKCHAN_UI_LISTENING || mode == STACKCHAN_UI_THINKING);
        fx_t f;
        fx_get(&f);
        int want = speaking ? L_SPEAK : happy ? L_PET : ui ? L_UI : f.fx != STACKCHAN_FX_NONE ? L_FX : L_BASE;
        bool changed = want != layer || led_force;
        layer = want;
        led_force = false;
        switch (want) {
        case L_SPEAK: {   // brightness follows the voice, like a mouth
            float l = s_level;
            smooth = l > smooth ? 0.5f * smooth + 0.5f * l : 0.75f * smooth + 0.25f * l;   // fast up, slow down
            int br = 30 + (int)(smooth * 225.0f);
            if (changed) shown = -1;
            if (shown < 0 || br - shown >= 8 || shown - br >= 8) {
                frame_fill((uint8_t)(SPEAK_R * br / 255), (uint8_t)(SPEAK_G * br / 255), (uint8_t)(SPEAK_B * br / 255));
                frame_send();
                shown = br;
            }
            break;
        }
        case L_PET:
            if (spinning || joy >= JOY_SPIN) {   // overjoyed: rainbow
                fx_t r = {.fx = STACKCHAN_FX_RAINBOW, .speed = 8};
                render_fx(&r, now);
                frame_send();
            } else if (joy >= JOY_WAG) {        // happy: pink breathing
                fx_t b = {.fx = STACKCHAN_FX_BREATH, .r = PET_R, .g = PET_G, .b = PET_B, .speed = 8};
                render_fx(&b, now);
                frame_send();
                shown = -2;
            } else if (changed || shown == -2) {
                frame_fill(PET_R, PET_G, PET_B);
                frame_send();
                shown = -1;
            }
            break;
        case L_UI: {      // listening: blue breath; thinking: yellow dots going round (as HermesLLM)
            fx_t u = {0};
            if (mode == STACKCHAN_UI_LISTENING) {
                u.fx = STACKCHAN_FX_BREATH;
                u.b = 200;
                u.speed = 7;
            } else {
                u.fx = STACKCHAN_FX_CHASE;
                u.r = 180;
                u.g = 160;
                u.speed = 6;
            }
            render_fx(&u, now);
            frame_send();
            break;
        }
        case L_FX:
            if (f.fx != STACKCHAN_FX_SOLID || changed || s_fx_dirty) {
                s_fx_dirty = false;
                render_fx(&f, now);
                frame_send();
            }
            break;
        default:
            if (changed || s_base_dirty) {
                s_base_dirty = false;
                xSemaphoreTake(s_lock, portMAX_DELAY);
                memcpy(s_led_ram, s_base, sizeof(s_led_ram));
                xSemaphoreGive(s_lock);
                frame_send();
            }
            break;
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
    memset(s_base, 0, sizeof(s_base));
    s_led_ready = true;
    frame_send();

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
    if (si12t_setup(bus) == ESP_OK) {
        s_touch_ready = true;
        ESP_LOGI(TAG, "head touch ready");
    } else {
        ESP_LOGW(TAG, "head touch (Si12T) not found");
    }
    calib_load();
    s_uart_ok = true;
    int tries = 0;
    while (!servo_probe() && ++tries < 12) vTaskDelay(pdMS_TO_TICKS(150));   // up to ~2 s
    // A restart in the middle of a full turn leaves X in wheel mode (the servos keep their
    // power across our restart); position writes would then set its speed. Back to positions.
    if (s_servo_ready) spin_x(false, 0);
    if (s_servo_ready) ESP_LOGI(TAG, "servos answered (try %d)", tries + 1);
    else ESP_LOGW(TAG, "servos not answering yet; will keep trying every 5 s");
    return ESP_OK;
}

bool stackchan_led_ready(void) { return s_led_ready; }
bool stackchan_servo_ready(void) { return s_servo_ready; }

// Base colours: shown when no other layer is active. Setting them stops an effect.
esp_err_t stackchan_led_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (!s_led_ready) return ESP_ERR_INVALID_STATE;
    if (index >= STACKCHAN_LED_COUNT) return ESP_ERR_INVALID_ARG;
    uint16_t v = rgb565(r, g, b);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_base[index * 2] = (uint8_t)(v & 0xFF);
    s_base[index * 2 + 1] = (uint8_t)(v >> 8);
    xSemaphoreGive(s_lock);
    fx_t off = {0};
    fx_put(&off);
    return ESP_OK;
}

esp_err_t stackchan_led_show(void) {
    if (!s_led_ready) return ESP_ERR_INVALID_STATE;
    base_changed();
    return ESP_OK;
}

esp_err_t stackchan_led_fill(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < STACKCHAN_LED_COUNT; i++) {
        esp_err_t err = stackchan_led_set(i, r, g, b);
        if (err != ESP_OK) return err;
    }
    return stackchan_led_show();
}

int stackchan_led_effect_from_name(const char *name) {
    if (!name) return -1;
    if (!strcmp(name, "strobe")) return STACKCHAN_FX_BLINK;
    for (int i = 0; i < (int)(sizeof(k_fx_names) / sizeof(k_fx_names[0])); i++) {
        if (!strcmp(name, k_fx_names[i])) return i;
    }
    return -1;
}

esp_err_t stackchan_led_effect(const char *name, uint8_t r, uint8_t g, uint8_t b,
                               uint8_t r2, uint8_t g2, uint8_t b2, int speed) {
    if (!s_led_ready) return ESP_ERR_INVALID_STATE;
    int fx = stackchan_led_effect_from_name(name);
    if (fx < 0) return ESP_ERR_NOT_FOUND;
    bool strobe = !strcmp(name, "strobe");
    fx_t f = {.fx = fx, .r = r, .g = g, .b = b, .r2 = r2, .g2 = g2, .b2 = b2, .speed = speed};
    if (f.speed <= 0) f.speed = strobe ? 8 : 5;
    if (!(r | g | b)) {   // no colour given: a sensible one for the effect
        switch (fx) {
        case STACKCHAN_FX_BREATH: f.r = 0; f.g = 120; f.b = 200; break;
        case STACKCHAN_FX_CHASE: f.r = 255; f.g = 140; f.b = 0; break;
        case STACKCHAN_FX_WAVE: f.r = 0; f.g = 80; f.b = 200; break;
        case STACKCHAN_FX_ALTERNATE: f.r = 255; f.g = 0; f.b = 0; break;
        default: f.r = 255; f.g = 255; f.b = 255; break;
        }
    }
    if (fx == STACKCHAN_FX_ALTERNATE && !(r2 | g2 | b2)) {
        f.r2 = 0;
        f.g2 = 200;
        f.b2 = 0;
    }
    fx_put(&f);
    return ESP_OK;
}

esp_err_t stackchan_led_preset(const char *name) {
    if (!name) return ESP_ERR_NOT_FOUND;
    // The HermesLLM presets.
    if (!strcmp(name, "warm_white")) return stackchan_led_effect("solid", 255, 200, 120, 0, 0, 0, 5);
    if (!strcmp(name, "cool_blue")) return stackchan_led_effect("solid", 0, 100, 255, 0, 0, 0, 5);
    if (!strcmp(name, "alert_red")) return stackchan_led_effect("blink", 255, 0, 0, 0, 0, 0, 3);
    if (!strcmp(name, "calm_breath")) return stackchan_led_effect("breath", 0, 120, 200, 0, 0, 0, 5);
    if (!strcmp(name, "christmas")) return stackchan_led_effect("alternate", 255, 0, 0, 0, 200, 0, 7);
    if (!strcmp(name, "ocean")) return stackchan_led_effect("wave", 0, 80, 200, 0, 0, 0, 5);
    if (!strcmp(name, "rainbow")) return stackchan_led_effect("rainbow", 0, 0, 0, 0, 0, 0, 5);
    if (!strcmp(name, "party")) {
        return stackchan_led_effect("chase", (uint8_t)rand_between(100, 255), (uint8_t)rand_between(100, 255),
                                    (uint8_t)rand_between(100, 255), 0, 0, 0, 9);
    }
    return ESP_ERR_NOT_FOUND;
}

void stackchan_hw_ui_mode(int muse_mode) { s_ui_mode = muse_mode; }
void stackchan_led_ui_effects(bool on) { s_ui_fx_on = on; }

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

// Where the head is going: the springs' targets. (A separate "last commanded" copy went stale
// when the springs were aimed directly, e.g. home after a full turn, and the next pat then
// reacted around the old angle, turning the head 90 degrees to one side.)
void stackchan_servo_position(int *x_deg, int *y_deg) {
    taskENTER_CRITICAL(&s_spring_mux);
    float tx = s_spx.target, ty = s_spy.target;
    taskEXIT_CRITICAL(&s_spring_mux);
    if (x_deg) *x_deg = (int)lroundf(tx);
    if (y_deg) *y_deg = (int)lroundf(ty);
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
    } else if (!strncmp(line, "sway=", 5)) {   // >sway=DEG,HALF_MS[,UP]  tune the speech sway
        c = (int)s_sway_up;
        if (sscanf(line + 5, "%d,%d,%d", &a, &b, &c) >= 2 && a >= 0 && a <= 40 && b >= 200 && b <= 3000 &&
            c >= 0 && c <= 20) {
            s_sway_deg = (float)a;
            s_sway_half_ms = (uint32_t)b;
            s_sway_up = (float)c;
        }
        printf("@sway %d deg, %u ms per swing, %d deg up\n", (int)s_sway_deg, (unsigned)s_sway_half_ms,
               (int)s_sway_up);
    } else if (!strncmp(line, "swaytest=", 9)) {   // >swaytest=SECONDS  sway without speech
        a = atoi(line + 9);
        if (a < 1) a = 1;
        if (a > 30) a = 30;
        s_fake_voice_until = now_ms() + (uint32_t)a * 1000;
        s_fake_voice = true;
        printf("@swaytest %d s\n", a);
    } else if (!strcmp(line, "touch")) {   // debug: the last head-touch reading
        uint8_t v = s_touch_raw;
        printf("@touch ready=%d raw=0x%02X front=%u mid=%u back=%u\n", s_touch_ready, v, v & 3, (v >> 2) & 3, (v >> 4) & 3);
    } else if (!strncmp(line, "stroke=", 7)) {   // >stroke=N  as if the head were stroked N times
        a = atoi(line + 7);
        s_fake_strokes = a < 1 ? 1 : (a > 10 ? 10 : a);
        printf("@stroke %d\n", s_fake_strokes);
    } else if (!strncmp(line, "pet=", 4)) {
        s_pet_on = line[4] == '1';
        printf("@pet %s\n", s_pet_on ? "on" : "off");
    } else if (!strncmp(line, "ledfx=", 6)) {   // >ledfx=NAME[,SPEED[,R,G,B[,R2,G2,B2]]]
        char name[16] = {0};
        int sp = 0, r = 0, g = 0, bb = 0, r2 = 0, g2 = 0, b2 = 0;
        int n = sscanf(line + 6, "%15[a-z_],%d,%d,%d,%d,%d,%d,%d", name, &sp, &r, &g, &bb, &r2, &g2, &b2);
        esp_err_t err = n >= 1 ? stackchan_led_effect(name, (uint8_t)clampi(r, 0, 255), (uint8_t)clampi(g, 0, 255),
                                                       (uint8_t)clampi(bb, 0, 255), (uint8_t)clampi(r2, 0, 255),
                                                       (uint8_t)clampi(g2, 0, 255), (uint8_t)clampi(b2, 0, 255), sp)
                               : ESP_ERR_INVALID_ARG;
        printf("@ledfx %s %s\n", name, err == ESP_OK ? "ok" : esp_err_to_name(err));
    } else if (!strncmp(line, "ledpreset=", 10)) {
        esp_err_t err = stackchan_led_preset(line + 10);
        printf("@ledpreset %s %s\n", line + 10, err == ESP_OK ? "ok" : esp_err_to_name(err));
    } else if (!strncmp(line, "uifx=", 5)) {   // >uifx=0|1  listening / thinking LEDs
        stackchan_led_ui_effects(line[5] == '1');
        printf("@uifx %s\n", line[5] == '1' ? "on" : "off");
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
        if (!strcmp(v, "pos")) {   // debug: where the servos really are, and what the springs hold
            float rx = -2, ry = -2;
            if (s_uart_ok) {
                xSemaphoreTake(s_lock, portMAX_DELAY);
                rx = scs_read_deg(SCS_ID_X);
                ry = scs_read_deg(SCS_ID_Y);
                xSemaphoreGive(s_lock);
            }
            taskENTER_CRITICAL(&s_spring_mux);
            spring_t sx = s_spx, sy = s_spy;
            taskEXIT_CRITICAL(&s_spring_mux);
            printf("@servo pos read x=%.1f y=%.1f | spring x=%.1f->%.1f%s y=%.1f->%.1f%s | ready=%d\n",
                   rx, ry, sx.pos, sx.target, sx.active ? "*" : "", sy.pos, sy.target, sy.active ? "*" : "",
                   s_servo_ready);
            fflush(stdout);
            return true;
        }
        float tdx, tdy;
        if (sscanf(v, "trim=%f,%f", &tdx, &tdy) == 2 && fabsf(tdx) <= 30 && fabsf(tdy) <= 30) {
            // >servo=trim=DX,DY: move the centre by exactly DX/DY degrees (Y smaller = up), without
            // reading the servos, whose resting position wanders by a degree or two.
            s_off_x += tdx;
            s_off_y += tdy;
            err = calib_save();
            if (s_servo_ready) motion_teleport_to_present();
            printf("@servo trim %s: offset x%+.1f y%+.1f deg\n", err == ESP_OK ? "saved" : esp_err_to_name(err),
                   s_off_x, s_off_y);
            fflush(stdout);
            return true;
        }
        if (!strcmp(v, "zero") || !strcmp(v, "zero_reset")) {
            // >servo=zero: where the head is now becomes the centre (turn the torque off and
            // set it by hand first). >servo=zero_reset forgets the calibration.
            if (!s_servo_ready) {
                printf("@servo zero: servos not ready\n");
                fflush(stdout);
                return true;
            }
            bool reset = v[4] == '_';
            float rx = 0, ry = 0;
            if (!reset) {
                xSemaphoreTake(s_lock, portMAX_DELAY);
                rx = scs_read_deg(SCS_ID_X);   // calibrated, with the current offsets
                ry = scs_read_deg(SCS_ID_Y);
                xSemaphoreGive(s_lock);
                if (rx < 0 || ry < 0) {
                    printf("@servo zero: could not read the servos\n");
                    fflush(stdout);
                    return true;
                }
            }
            float raw_x = rx + s_off_x, raw_y = ry + s_off_y;   // the servo's own angles
            s_off_x = reset ? 0 : raw_x - STACKCHAN_SERVO_X_CENTER;
            s_off_y = reset ? 0 : raw_y - STACKCHAN_SERVO_Y_CENTER;
            err = calib_save();
            // The springs now hold the same physical place in the new angles.
            motion_teleport_to_present();
            printf("@servo zero %s: offset x%+.1f y%+.1f deg\n", err == ESP_OK ? "saved" : esp_err_to_name(err),
                   s_off_x, s_off_y);
            fflush(stdout);
            return true;
        }
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

void stackchan_hw_set_pet_callback(void (*cb)(void)) { s_pet_cb = cb; }
