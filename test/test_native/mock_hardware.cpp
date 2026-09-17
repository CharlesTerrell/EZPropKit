#include "mock_hardware.h"
#include "Arduino.h"
#include "FatFS.h"
#include "Wire.h"
#include "Adafruit_LIS3DH.h"
#include "pico/util/queue.h"
#include "core1_engine.h"
#include "msc_disk.h"

#include <stdlib.h>
#include <string.h>

// Global mock instances
MockSerial Serial;
TwoWire Wire;
MockFatFS FatFS;

// Pin state tracking
static uint8_t s_pin_mode[32] = {0};
static int s_pin_digital[32] = {0};
static int s_pin_analog[32] = {0};
static bool s_pin_driven[32] = {false};
static int s_analog_res = 10;
static int s_analog_range = 255;
static uint32_t s_simulated_millis = 1000;

// Subsystem mock states
static bool s_audio_playing = false;
static bool s_reload_pending = false;

// LIS3DH mock static members
float Adafruit_LIS3DH::s_mock_x = 0.0f;
float Adafruit_LIS3DH::s_mock_y = 0.0f;
float Adafruit_LIS3DH::s_mock_z = 1.0f;
uint8_t Adafruit_LIS3DH::s_mock_click = 0;
bool Adafruit_LIS3DH::s_mock_begin_ok = true;

Adafruit_LIS3DH::Adafruit_LIS3DH(TwoWire* wire) {
    (void)wire;
}

bool Adafruit_LIS3DH::begin(uint8_t addr) {
    (void)addr;
    return s_mock_begin_ok;
}

void Adafruit_LIS3DH::setRange(int range) {
    (void)range;
}

bool Adafruit_LIS3DH::getEvent(sensors_event_t* event) {
    if (!event) return false;
    event->acceleration.x = s_mock_x;
    event->acceleration.y = s_mock_y;
    event->acceleration.z = s_mock_z;
    return true;
}

uint8_t Adafruit_LIS3DH::getClick() {
    return s_mock_click;
}

void Adafruit_LIS3DH::mock_set_accel(float x, float y, float z) {
    s_mock_x = x;
    s_mock_y = y;
    s_mock_z = z;
}

void Adafruit_LIS3DH::mock_set_click(uint8_t click) {
    s_mock_click = click;
}

void Adafruit_LIS3DH::mock_set_begin_success(bool success) {
    s_mock_begin_ok = success;
}

// Arduino functions
void pinMode(uint8_t pin, uint8_t mode) {
    if (pin < 32) {
        s_pin_mode[pin] = mode;
        if (!s_pin_driven[pin]) {
            if (mode == INPUT_PULLUP) {
                s_pin_digital[pin] = HIGH;
            } else if (mode == INPUT_PULLDOWN) {
                s_pin_digital[pin] = LOW;
            }
        }
    }
}

void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin < 32) {
        s_pin_digital[pin] = (val != 0) ? HIGH : LOW;
        s_pin_driven[pin] = true;
    }
}

int digitalRead(uint8_t pin) {
    if (pin < 32) {
        return s_pin_digital[pin];
    }
    return LOW;
}

int analogRead(uint8_t pin) {
    if (pin < 32) {
        return s_pin_analog[pin];
    }
    return 0;
}

void analogWrite(uint8_t pin, int val) {
    if (pin < 32) {
        s_pin_analog[pin] = val;
    }
}

void analogReadResolution(int bits) {
    s_analog_res = bits;
}

void analogWriteRange(int range) {
    s_analog_range = range;
}

uint32_t millis(void) {
    return s_simulated_millis;
}

uint32_t micros(void) {
    return s_simulated_millis * 1000UL;
}

void delay(uint32_t ms) {
    s_simulated_millis += ms;
}

void delayMicroseconds(uint32_t us) {
    s_simulated_millis += (us / 1000);
}

void yield(void) {
    // No-op in native test
}

// Mock Pico queue implementation
void queue_init(queue_t *q, uint element_size, uint element_count) {
    if (!q) return;
    q->element_size = element_size;
    q->element_count = element_count;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->buffer = (uint8_t*)calloc(element_count, element_size);
}

bool queue_try_add(queue_t *q, const void *data) {
    if (!q || !q->buffer || !data) return false;
    if (q->count >= q->element_count) return false;

    memcpy(q->buffer + (q->head * q->element_size), data, q->element_size);
    q->head = (q->head + 1) % q->element_count;
    q->count++;
    return true;
}

bool queue_try_remove(queue_t *q, void *data) {
    if (!q || !q->buffer || !data) return false;
    if (q->count == 0) return false;

    memcpy(data, q->buffer + (q->tail * q->element_size), q->element_size);
    q->tail = (q->tail + 1) % q->element_count;
    q->count--;
    return true;
}

void queue_free(queue_t *q) {
    if (q && q->buffer) {
        free(q->buffer);
        q->buffer = nullptr;
        q->count = 0;
    }
}

uint queue_get_level(queue_t *q) {
    return q ? q->count : 0;
}

// Stubs for Core 1 and MSC Disk
bool core1_audio_is_playing(void) {
    return s_audio_playing;
}

void msc_disk_task(void) {
    // Stub
}

bool msc_disk_is_reload_pending(void) {
    return s_reload_pending;
}

// Test helper implementations
void mock_hardware_reset(void) {
    memset(s_pin_mode, 0, sizeof(s_pin_mode));
    memset(s_pin_digital, 0, sizeof(s_pin_digital));
    memset(s_pin_analog, 0, sizeof(s_pin_analog));
    memset(s_pin_driven, 0, sizeof(s_pin_driven));
    s_simulated_millis = 1000;
    s_audio_playing = false;
    s_reload_pending = false;
    Adafruit_LIS3DH::mock_set_accel(0.0f, 0.0f, 1.0f);
    Adafruit_LIS3DH::mock_set_click(0);
    Adafruit_LIS3DH::mock_set_begin_success(true);
    FatFS.clear();
}

void mock_set_millis(uint32_t ms) {
    s_simulated_millis = ms;
}

void mock_advance_millis(uint32_t ms) {
    s_simulated_millis += ms;
}

void mock_set_digital_pin(uint8_t pin, int val) {
    if (pin < 32) {
        s_pin_digital[pin] = val;
        s_pin_driven[pin] = true;
    }
}

int mock_get_digital_pin(uint8_t pin) {
    return (pin < 32) ? s_pin_digital[pin] : LOW;
}

uint8_t mock_get_pin_mode(uint8_t pin) {
    return (pin < 32) ? s_pin_mode[pin] : 0;
}

void mock_set_analog_pin(uint8_t pin, int val) {
    if (pin < 32) s_pin_analog[pin] = val;
}

int mock_get_analog_pin(uint8_t pin) {
    return (pin < 32) ? s_pin_analog[pin] : 0;
}

void mock_set_audio_playing(bool playing) {
    s_audio_playing = playing;
}

void mock_set_reload_pending(bool pending) {
    s_reload_pending = pending;
}
