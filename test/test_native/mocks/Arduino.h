#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <algorithm>

typedef bool boolean;

#define HIGH 1
#define LOW  0

#define INPUT           0
#define OUTPUT          1
#define INPUT_PULLUP    2
#define INPUT_PULLDOWN  3

#ifdef __cplusplus
extern "C" {
#endif

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);
void analogReadResolution(int bits);
void analogWriteRange(int range);

uint32_t millis(void);
uint32_t micros(void);
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
void yield(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class MockSerial {
public:
    void begin(int baud) { (void)baud; }
    void print(const char* s) { if (s) fputs(s, stdout); }
    void print(int val) { printf("%d", val); }
    void print(float val) { printf("%f", val); }
    void println(const char* s = "") { if (s) puts(s); else putchar('\n'); }
    void println(int val) { printf("%d\n", val); }
    void println(float val) { printf("%f\n", val); }
    int printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        int ret = vprintf(format, args);
        va_end(args);
        return ret;
    }
};

extern MockSerial Serial;
#endif
