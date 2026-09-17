#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void mock_hardware_reset(void);
void mock_set_millis(uint32_t ms);
void mock_advance_millis(uint32_t ms);

void mock_set_digital_pin(uint8_t pin, int val);
int mock_get_digital_pin(uint8_t pin);
uint8_t mock_get_pin_mode(uint8_t pin);

void mock_set_analog_pin(uint8_t pin, int val);
int mock_get_analog_pin(uint8_t pin);

void mock_set_audio_playing(bool playing);
void mock_set_reload_pending(bool pending);

#ifdef __cplusplus
}
#endif
