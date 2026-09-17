#pragma once

#include <stdint.h>
#include <stdbool.h>

#define LIS3DH_RANGE_4_G 1

typedef struct {
    struct {
        float x;
        float y;
        float z;
    } acceleration;
} sensors_event_t;

class TwoWire;

class Adafruit_LIS3DH {
public:
    Adafruit_LIS3DH(TwoWire* wire = nullptr);
    bool begin(uint8_t addr);
    void setRange(int range);
    bool getEvent(sensors_event_t* event);
    uint8_t getClick();

    static void mock_set_accel(float x, float y, float z);
    static void mock_set_click(uint8_t click);
    static void mock_set_begin_success(bool success);

private:
    static float s_mock_x;
    static float s_mock_y;
    static float s_mock_z;
    static uint8_t s_mock_click;
    static bool s_mock_begin_ok;
};
