#pragma once

#include <stdint.h>

class TwoWire {
public:
    void setSDA(uint8_t pin) { (void)pin; }
    void setSCL(uint8_t pin) { (void)pin; }
    void begin() {}
};

extern TwoWire Wire;
