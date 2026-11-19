#include "stackchan_hw.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_log.h"
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
#define REG_LED_CFG       0x24   // bits 0..5 count, bit 6 refresh
#define REG_LED_RAM_START 0x30   // RGB565, little endian, 2 bytes per LED
#define SERVO_DIR_PIN     0      // half-duplex direction pin on the PY32

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

static i2c_master_dev_handle_t s_py32;
static SemaphoreHandle_t s_lock;
static bool s_led_ready;
static bool s_servo_ready;
static bool s_inited;
static uint8_t s_led_ram[STACKCHAN_LED_COUNT * 2];
static int s_x = STACKCHAN_SERVO_X_CENTER;
static int s_y = STACKCHAN_SERVO_Y_CENTER;

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

static uint16_t scs_pos(int deg) {
    if (deg < 0) deg = 0;
    if (deg > 300) deg = 300;
    return (uint16_t)(1023 - (deg * 1023 + 150) / 300);   // same mapping as HermesLLM (inverted)
}

static void scs_write_pos(uint8_t id, int deg, uint32_t ms) {
    uint16_t pos = scs_pos(deg);
    if (ms > 0xFFFF) ms = 0xFFFF;
    // SCSCL is big endian: position, time, speed (0 = use time)
    uint8_t d[6] = {(uint8_t)(pos >> 8), (uint8_t)pos, (uint8_t)(ms >> 8), (uint8_t)ms, 0, 0};
    scs_send(id, SCS_INST_WRITE, SCSCL_GOAL_POSITION_L, d, sizeof(d), true);
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

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

    // LEDs
    py32_write8(REG_LED_CFG, STACKCHAN_LED_COUNT & 0x3F);
    memset(s_led_ram, 0, sizeof(s_led_ram));
    s_led_ready = true;
    stackchan_led_show();

    // Servo bus: PY32 pin 0 high (output, pulled up) enables the half-duplex line, then the UART.
    py32_bit(REG_GPIO_M_L, SERVO_DIR_PIN, true);
    py32_bit(REG_GPIO_PD_L, SERVO_DIR_PIN, false);
    py32_bit(REG_GPIO_PU_L, SERVO_DIR_PIN, true);
    py32_bit(REG_GPIO_O_L, SERVO_DIR_PIN, true);
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
    bool x_ok = scs_ping(SCS_ID_X), y_ok = scs_ping(SCS_ID_Y);
    ESP_LOGI(TAG, "servo ping: X=%d Y=%d", x_ok, y_ok);
    s_servo_ready = x_ok && y_ok;
    if (!s_servo_ready) ESP_LOGW(TAG, "servos not answering; servo commands disabled");
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
    xSemaphoreTake(s_lock, portMAX_DELAY);
    scs_write_pos(SCS_ID_X, x_deg, ms);
    scs_write_pos(SCS_ID_Y, y_deg, ms);
    s_x = x_deg;
    s_y = y_deg;
    xSemaphoreGive(s_lock);
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
