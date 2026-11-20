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

// LED effects (#11). The LEDs show the top active layer: speech > head pat > listening /
// thinking > an effect > the base colours above. Setting base colours stops an effect.
enum {
    STACKCHAN_FX_NONE = 0,   // "off"
    STACKCHAN_FX_SOLID,
    STACKCHAN_FX_BLINK,      // "strobe" is blink, faster by default
    STACKCHAN_FX_BREATH,
    STACKCHAN_FX_CHASE,
    STACKCHAN_FX_RAINBOW,
    STACKCHAN_FX_WAVE,
    STACKCHAN_FX_ALTERNATE,  // two colours swapping
};
int stackchan_led_effect_from_name(const char *name);   // -1 if unknown
// name: off, solid, blink, strobe, breath, chase, rainbow, wave, alternate. Colour 0,0,0 picks
// one that suits the effect; speed 1..10, 0 for the default. ESP_ERR_NOT_FOUND: unknown name.
esp_err_t stackchan_led_effect(const char *name, uint8_t r, uint8_t g, uint8_t b,
                               uint8_t r2, uint8_t g2, uint8_t b2, int speed);
// warm_white, cool_blue, alert_red, calm_breath, christmas, ocean, rainbow, party.
esp_err_t stackchan_led_preset(const char *name);

// The Muse UI mode (muse_mode_t), so listening and thinking can light up. Values match it.
#define STACKCHAN_UI_LISTENING 2
#define STACKCHAN_UI_THINKING  3
void stackchan_hw_ui_mode(int muse_mode);
void stackchan_led_ui_effects(bool on);

// Called when the head is stroked, and every second while the robot stays happy, so the face
// can show it. Muse registers muse_state_make_happy() (hop, blush, hearts).
void stackchan_hw_set_pet_callback(void (*cb)(void));

// Servos. Out-of-range angles are clamped to the soft limits, not rejected.
esp_err_t stackchan_servo_move(int x_deg, int y_deg, uint32_t ms);
esp_err_t stackchan_servo_center(uint32_t ms);
esp_err_t stackchan_servo_torque(bool on);
void stackchan_servo_position(int *x_deg, int *y_deg);   // last commanded

// Speech animation (#10): muse_audio_write() calls this with the 0..1 level of each chunk
// it plays. While speaking, the LEDs follow the level and the head sways; on by default.
void stackchan_hw_audio_level(float level);
void stackchan_hw_set_speech_anim(bool on);

// Serial console test commands, without the Muse app:
//   >ledfx=NAME[,SPEED[,R,G,B[,R2,G2,B2]]]   >ledpreset=NAME   >uifx=0|1
//   >led=R,G,B   >servo=X,Y[,MS]   >servo=center   >servo=off (torque off)   >servo=on   >anim=0|1
// Returns true if the line was one of these.
bool stackchan_hw_console_command(const char *line);

#ifdef __cplusplus
}
#endif
