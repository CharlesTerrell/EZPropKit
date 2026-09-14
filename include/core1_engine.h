#pragma once

#include <Arduino.h>
#include "ipc_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void core1_start(void);
void core1_loop_process(void);
bool core1_audio_is_playing(void);

#ifdef __cplusplus
}
#endif
