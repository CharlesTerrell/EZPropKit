#pragma once

#include <stdint.h>
#include <vector>

#define NEO_GRB 0
#define NEO_KHZ800 0

class Adafruit_NeoPixel {
public:
    Adafruit_NeoPixel(uint16_t n = 0, int16_t pin = -1, uint16_t type = 0)
        : _num_pixels(n), _pin(pin), _brightness(255), _pixels(n, 0) {
        (void)type;
    }

    virtual ~Adafruit_NeoPixel() = default;

    void begin() {}

    void show() {}

    void setPixelColor(uint16_t n, uint32_t c) {
        if (n < _pixels.size()) {
            _pixels[n] = c;
        }
    }

    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
        setPixelColor(n, Color(r, g, b));
    }

    void fill(uint32_t c = 0, uint16_t first = 0, uint16_t count = 0) {
        if (first >= _pixels.size()) return;
        size_t end = (count == 0) ? _pixels.size() : std::min((size_t)(first + count), _pixels.size());
        for (size_t i = first; i < end; ++i) {
            _pixels[i] = c;
        }
    }

    void setBrightness(uint8_t b) {
        _brightness = b;
    }

    uint8_t getBrightness() const {
        return _brightness;
    }

    void clear() {
        std::fill(_pixels.begin(), _pixels.end(), 0);
    }

    uint16_t numPixels() const {
        return (uint16_t)_pixels.size();
    }

    static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    uint32_t getPixelColor(uint16_t n) const {
        if (n < _pixels.size()) {
            return _pixels[n];
        }
        return 0;
    }

private:
    uint16_t _num_pixels;
    int16_t _pin;
    uint8_t _brightness;
    std::vector<uint32_t> _pixels;
};
