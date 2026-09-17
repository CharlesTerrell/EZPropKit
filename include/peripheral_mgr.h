#pragma once

#include <Arduino.h>
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Power Rail Control (GPIO 23)
void power_rail_enable(void);
void power_rail_disable(void);
bool power_rail_is_enabled(void);

// Onboard Indicators
void led_set(bool on);
void led_toggle(void);
void status_pixel_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void status_pixel_set_brightness(uint8_t brightness);
void status_pixel_pulse_error(void);


// External NeoPixel Strip (Lazy Initialized)
bool neopixel_init(uint16_t num_pixels, uint8_t pin);
void neopixel_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void neopixel_fill(uint8_t r, uint8_t g, uint8_t b);
void neopixel_set_brightness(uint8_t brightness);
void neopixel_show(void);
void neopixel_clear(void);

// Servo PWM (Lazy Initialized)
bool servo_init(uint8_t pin, uint16_t min_us, uint16_t max_us);
void servo_write_angle(float degrees);
void servo_write_us(uint16_t us);
void servo_detach(void);

// Inputs & Sensors
bool button_is_pressed(void);
bool boot_button_is_pressed(void);
float battery_read_voltage(void);
uint8_t battery_read_percent(void);

// Dynamic Runtime Buttons & LEDs on Any GPIO
void pin_button_init(uint8_t pin, uint8_t pull_mode);
bool pin_button_is_pressed(uint8_t pin, bool active_low);
void pin_led_init(uint8_t pin);
void pin_led_set(uint8_t pin, bool on, bool active_high);
void pin_led_toggle(uint8_t pin);
void pin_led_pwm(uint8_t pin, uint8_t duty);

// Generic GPIO Control
void pin_gpio_mode(uint8_t pin, uint8_t mode);
bool pin_gpio_read(uint8_t pin);
void pin_gpio_write(uint8_t pin, bool val);
int pin_gpio_adc(uint8_t pin);

// Accelerometer (LIS3DH)
bool motion_init(void);
bool motion_read_accel(float* x, float* y, float* z);
bool motion_is_tapped(void);

// System Teardown (Safe Reset)
void peripherals_teardown(void);

#ifdef __cplusplus
}
#endif
