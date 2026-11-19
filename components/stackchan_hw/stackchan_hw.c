#include "stackchan_hw.h"

#include <stdio.h>
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

static void bus_guard_task(void *arg) {
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(BUS_GUARD_MS));
        if (bus_5v_assert() && s_led_ready) {
            vTaskDelay(pdMS_TO_TICKS(20));   // LEDs lose their colour without power: send it again
            stackchan_led_show();
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
        if (xTaskCreate(bus_guard_task, "sc_bus5v", 2560, NULL, 2, NULL) != pdPASS) {
            ESP_LOGW(TAG, "5V guard task not started");
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
