#include "peripheral_mgr.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_LIS3DH.h>
#include <Wire.h>
#include <Servo.h>

// Static state variables
static bool power_enabled = false;
static Adafruit_NeoPixel* status_pixel = nullptr;
static Adafruit_NeoPixel* ext_pixels = nullptr;
static Adafruit_LIS3DH* accel = nullptr;

static Servo prop_servo;
static uint8_t servo_pwm_pin = 255;
static uint16_t servo_min_us = 500;
static uint16_t servo_max_us = 2500;
static bool servo_active = false;

// Power Rail Control
void power_rail_enable(void) {
    if (!power_enabled) {
        power_enabled = true;
        pinMode(PIN_EXTERNAL_POWER, OUTPUT);
        digitalWrite(PIN_EXTERNAL_POWER, HIGH);
        delay(POWER_ENABLE_DELAY_MS); // Prevent inrush brownout
    }
}

void power_rail_disable(void) {
    if (power_enabled) {
        digitalWrite(PIN_EXTERNAL_POWER, LOW);
        power_enabled = false;
        // Hold I2S lines LOW to ensure MAX98357A stays in shutdown without power
        pinMode(PIN_I2S_BCLK, OUTPUT);
        digitalWrite(PIN_I2S_BCLK, LOW);
        pinMode(PIN_I2S_DATA, OUTPUT);
        digitalWrite(PIN_I2S_DATA, LOW);
        pinMode(PIN_I2S_LRCLK, OUTPUT);
        digitalWrite(PIN_I2S_LRCLK, LOW);
    }
}

bool power_rail_is_enabled(void) {
    return power_enabled;
}

// Onboard Indicators
void led_set(bool on) {
    pinMode(PIN_ONBOARD_LED, OUTPUT);
    digitalWrite(PIN_ONBOARD_LED, on ? HIGH : LOW);
}

void led_toggle(void) {
    pinMode(PIN_ONBOARD_LED, OUTPUT);
    digitalWrite(PIN_ONBOARD_LED, !digitalRead(PIN_ONBOARD_LED));
}

void status_pixel_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    if (!status_pixel) {
        status_pixel = new Adafruit_NeoPixel(1, PIN_ONBOARD_NEOPIXEL, NEO_GRB + NEO_KHZ800);
        status_pixel->begin();
    }
    status_pixel->setPixelColor(0, status_pixel->Color(r, g, b));
    status_pixel->show();
}

void status_pixel_pulse_error(void) {
    // Breathing red/orange error beacon
    uint32_t ms = millis();
    float wave = (sinf((float)ms / 200.0f) + 1.0f) * 0.5f; // 0.0 to 1.0
    uint8_t r = (uint8_t)(wave * 200.0f + 55.0f);
    uint8_t g = (uint8_t)(wave * 40.0f);
    status_pixel_set_rgb(r, g, 0);
}

// External NeoPixel Strip
bool neopixel_init(uint16_t num_pixels, uint8_t pin) {
    power_rail_enable(); // Terminal NeoPixel requires 5V rail
    if (ext_pixels) {
        delete ext_pixels;
    }
    ext_pixels = new Adafruit_NeoPixel(num_pixels, pin, NEO_GRB + NEO_KHZ800);
    ext_pixels->begin();
    ext_pixels->clear();
    ext_pixels->show();
    return true;
}

void neopixel_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (ext_pixels && index < ext_pixels->numPixels()) {
        ext_pixels->setPixelColor(index, ext_pixels->Color(r, g, b));
    }
}

void neopixel_fill(uint8_t r, uint8_t g, uint8_t b) {
    if (ext_pixels) {
        ext_pixels->fill(ext_pixels->Color(r, g, b));
    }
}

void neopixel_set_brightness(uint8_t brightness) {
    if (ext_pixels) {
        ext_pixels->setBrightness(brightness);
    }
}

void neopixel_show(void) {
    if (ext_pixels) {
        ext_pixels->show();
    }
}

void neopixel_clear(void) {
    if (ext_pixels) {
        ext_pixels->clear();
        ext_pixels->show();
    }
}

// Servo PWM Control
bool servo_init(uint8_t pin, uint16_t min_us, uint16_t max_us) {
    power_rail_enable(); // Servo power requires 5V rail
    servo_pwm_pin = pin;
    servo_min_us = min_us;
    servo_max_us = max_us;

    if (prop_servo.attached()) {
        prop_servo.detach();
    }
    prop_servo.attach(servo_pwm_pin, servo_min_us, servo_max_us, 1500);
    servo_active = true;
    return true;
}

void servo_write_angle(float degrees) {
    if (!servo_active || !prop_servo.attached()) return;
    if (degrees < 0.0f) degrees = 0.0f;
    if (degrees > 180.0f) degrees = 180.0f;

    prop_servo.write((int)roundf(degrees));
}

void servo_write_us(uint16_t us) {
    if (!servo_active || !prop_servo.attached()) return;
    prop_servo.writeMicroseconds(us);
}

void servo_detach(void) {
    if (servo_active || prop_servo.attached()) {
        prop_servo.detach();
        pinMode(servo_pwm_pin, OUTPUT);
        digitalWrite(servo_pwm_pin, LOW);
        servo_active = false;
    }
}

// Inputs & Sensors
bool button_is_pressed(void) {
    pinMode(PIN_EXTERNAL_BUTTON, INPUT_PULLUP);
    return digitalRead(PIN_EXTERNAL_BUTTON) == LOW;
}

bool boot_button_is_pressed(void) {
    pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
    return digitalRead(PIN_BOOT_BUTTON) == LOW;
}

float battery_read_voltage(void) {
    analogReadResolution(12);
    int raw = analogRead(PIN_BATTERY_ADC);
    // ADC 12-bit is 0..4095, 3.3V reference, 1/2 voltage divider
    return ((float)raw / 4095.0f) * 3.3f * 2.0f;
}

uint8_t battery_read_percent(void) {
    float v = battery_read_voltage();
    if (v >= 4.20f) return 100;
    if (v <= 3.20f) return 0;
    // Linear approximation between 3.2V and 4.2V
    return (uint8_t)(((v - 3.20f) / (4.20f - 3.20f)) * 100.0f);
}

// Motion Sensor (LIS3DH)
bool motion_init(void) {
    if (!accel) {
        Wire.setSDA(PIN_I2C_SDA);
        Wire.setSCL(PIN_I2C_SCL);
        Wire.begin();
        accel = new Adafruit_LIS3DH(&Wire);
        if (!accel->begin(0x18) && !accel->begin(0x19)) {
            return false;
        }
        accel->setRange(LIS3DH_RANGE_4_G);
    }
    return true;
}

bool motion_read_accel(float* x, float* y, float* z) {
    if (!accel && !motion_init()) return false;
    sensors_event_t event;
    accel->getEvent(&event);
    if (x) *x = event.acceleration.x;
    if (y) *y = event.acceleration.y;
    if (z) *z = event.acceleration.z;
    return true;
}

bool motion_is_tapped(void) {
    if (!accel && !motion_init()) return false;
    uint8_t click = accel->getClick();
    return (click & 0x30) != 0;
}

// Track user-configured pins for teardown
static uint32_t user_claimed_pins_mask = 0;

void pin_button_init(uint8_t pin, uint8_t pull_mode) {
    if (pin >= 30) return;
    user_claimed_pins_mask |= (1UL << pin);
    if (pull_mode == 0) {
        pinMode(pin, INPUT_PULLUP);
    } else if (pull_mode == 1) {
        pinMode(pin, INPUT_PULLDOWN);
    } else {
        pinMode(pin, INPUT);
    }
}

bool pin_button_is_pressed(uint8_t pin, bool active_low) {
    if (pin >= 30) return false;
    int val = digitalRead(pin);
    return active_low ? (val == LOW) : (val == HIGH);
}

void pin_led_init(uint8_t pin) {
    if (pin >= 30) return;
    user_claimed_pins_mask |= (1UL << pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void pin_led_set(uint8_t pin, bool on, bool active_high) {
    if (pin >= 30) return;
    user_claimed_pins_mask |= (1UL << pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, active_high ? (on ? HIGH : LOW) : (on ? LOW : HIGH));
}

void pin_led_toggle(uint8_t pin) {
    if (pin >= 30) return;
    user_claimed_pins_mask |= (1UL << pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, !digitalRead(pin));
}

void pin_led_pwm(uint8_t pin, uint8_t duty) {
    if (pin >= 30) return;
    user_claimed_pins_mask |= (1UL << pin);
    pinMode(pin, OUTPUT);
    analogWriteRange(255);
    analogWrite(pin, duty);
}

// Generic GPIO Control
void pin_gpio_mode(uint8_t pin, uint8_t mode) {
    if (pin >= 30) return;
    user_claimed_pins_mask |= (1UL << pin);
    pinMode(pin, mode);
}

bool pin_gpio_read(uint8_t pin) {
    if (pin >= 30) return false;
    return digitalRead(pin) == HIGH;
}

void pin_gpio_write(uint8_t pin, bool val) {
    if (pin >= 30) return;
    user_claimed_pins_mask |= (1UL << pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, val ? HIGH : LOW);
}

int pin_gpio_adc(uint8_t pin) {
    if (pin < 26 || pin > 29) return 0;
    analogReadResolution(12);
    return analogRead(pin);
}

// System Teardown
void peripherals_teardown(void) {
    // 1. Detach servo
    servo_detach();

    // 2. Clear NeoPixels
    if (ext_pixels) {
        ext_pixels->clear();
        ext_pixels->show();
        delete ext_pixels;
        ext_pixels = nullptr;
    }

    // 3. Status Pixel off
    if (status_pixel) {
        status_pixel->clear();
        status_pixel->show();
    }

    // 4. Red LED off
    led_set(false);

    // 5. Reset all user-claimed dynamic GPIO pins to safe High-Z input state
    for (uint8_t p = 0; p < 30; p++) {
        if (user_claimed_pins_mask & (1UL << p)) {
            analogWrite(p, 0);
            pinMode(p, INPUT);
        }
    }
    user_claimed_pins_mask = 0;

    // 6. Cut power rail
    power_rail_disable();
}
