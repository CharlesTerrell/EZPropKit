#pragma once

#include <stdint.h>

class Servo {
public:
    Servo() : _attached(false), _pin(-1), _min(544), _max(2400), _us(1500), _angle(90) {}

    uint8_t attach(int pin, int min = 544, int max = 2400, int default_us = 1500) {
        _attached = true;
        _pin = pin;
        _min = min;
        _max = max;
        _us = default_us;
        return 1;
    }

    void detach() {
        _attached = false;
        _pin = -1;
    }

    void write(int value) {
        _angle = value;
        // Approximation of angle to microseconds for testing
        _us = _min + (int)((float)(_max - _min) * (float)value / 180.0f);
    }

    void writeMicroseconds(int value) {
        _us = value;
    }

    bool attached() const {
        return _attached;
    }

    int read() const {
        return _angle;
    }

    int readMicroseconds() const {
        return _us;
    }

    int getPin() const {
        return _pin;
    }

private:
    bool _attached;
    int _pin;
    int _min;
    int _max;
    int _us;
    int _angle;
};
