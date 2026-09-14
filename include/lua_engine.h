#pragma once

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// Lua engine lifecycle
bool lua_engine_init(void);
bool lua_engine_run_file(const char* filepath);
void lua_engine_stop(void);
bool lua_engine_has_error(void);
const char* lua_engine_get_error(void);
void lua_engine_clear_error(void);

// Non-blocking cooperative yield
void lua_engine_yield_sleep(uint32_t ms);
void lua_engine_pump_ipc(void);

#ifdef __cplusplus
}
#endif
