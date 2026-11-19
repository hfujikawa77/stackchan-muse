// Stack-chan (M5Stack CoreS3 + Stack-chan body) hardware: 12 RGB LEDs and the two SCS0009 servos.
// Both hang off the PY32 IO expander (I2C 0x6F) on the CoreS3 internal I2C bus, shared with the SDK.
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACKCHAN_LED_COUNT 12

// Servo angles are degrees, same convention as HermesLLM (SCS0009: 0..300, X centre 150, Y centre 90).
#define STACKCHAN_SERVO_X_CENTER 150
#define STACKCHAN_SERVO_Y_CENTER 90

// Soft limits, tighter than the mechanical range so a bad command cannot drive the head into its stops.
#define STACKCHAN_SERVO_X_MIN 60
#define STACKCHAN_SERVO_X_MAX 240
#define STACKCHAN_SERVO_Y_MIN 70
#define STACKCHAN_SERVO_Y_MAX 90

// Moves faster than this are stretched to it (ms).
#define STACKCHAN_SERVO_MIN_MOVE_MS 150

// Brings up the IO expander (LEDs) and, if it answers, the servos. Safe to call twice.
// Returns ESP_OK when the IO expander is up; servo_ready() tells whether the servos answered.
esp_err_t stackchan_hw_init(void);

bool stackchan_led_ready(void);
bool stackchan_servo_ready(void);

// LEDs. Pixels are buffered; stackchan_led_show() sends them.
esp_err_t stackchan_led_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
esp_err_t stackchan_led_fill(uint8_t r, uint8_t g, uint8_t b);   // set all + show
esp_err_t stackchan_led_show(void);

// Servos. Out-of-range angles are clamped to the soft limits, not rejected.
esp_err_t stackchan_servo_move(int x_deg, int y_deg, uint32_t ms);
esp_err_t stackchan_servo_center(uint32_t ms);
esp_err_t stackchan_servo_torque(bool on);
void stackchan_servo_position(int *x_deg, int *y_deg);   // last commanded

#ifdef __cplusplus
}
#endif
