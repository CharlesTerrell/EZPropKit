#include "lua_engine.h"
#include "board_config.h"
#include "ipc_protocol.h"
#include "peripheral_mgr.h"
#include "msc_disk.h"
#include <FatFS.h>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

static lua_State* L = nullptr;
static bool has_error = false;
static char error_msg[512] = {0};

void lua_engine_pump_ipc(void) {
    core_event_t evt;
    while (ipc_receive_event(&evt)) {
        if (evt.type == EVT_DEBUG_LOG) {
            Serial.printf("[CORE 1] %s\r\n", evt.text);
        } else if (evt.type == EVT_AUDIO_STARTED) {
            const char* dec_name = "Unknown";
            if (evt.code == 1) dec_name = "WAV";
            else if (evt.code == 2) dec_name = "MP3";
            else if (evt.code == 3) dec_name = "AAC";
            else if (evt.code == 4) dec_name = "TONE";
            Serial.printf("[AUDIO] Playback started (codec: %s)\r\n", dec_name);
        } else if (evt.type == EVT_AUDIO_FINISHED) {
            Serial.println("[AUDIO] Playback finished.");
        } else if (evt.type == EVT_AUDIO_ERROR) {
            Serial.printf("[AUDIO ERROR] Code: %ld\r\n", (long)evt.code);
        }
    }
}

// Cooperative yield sleep for Lua
void lua_engine_yield_sleep(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
        yield();
        msc_disk_task();
        lua_engine_pump_ipc();
        if (msc_disk_is_reload_pending()) {
            if (L) {
                luaL_error(L, "__PROP_RELOAD__");
            }
            return;
        }
        delay(1);
    }
}

static void lua_count_hook(lua_State* L_hook, lua_Debug* ar) {
    (void)ar;
    yield();
    msc_disk_task();
    lua_engine_pump_ipc();
    if (msc_disk_is_reload_pending()) {
        luaL_error(L_hook, "__PROP_RELOAD__");
    }
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.power
// -----------------------------------------------------------------------------
static int l_prop_power_enable(lua_State* L) {
    power_rail_enable();
    return 0;
}

static int l_prop_power_disable(lua_State* L) {
    power_rail_disable();
    return 0;
}

static int l_prop_power_is_enabled(lua_State* L) {
    lua_pushboolean(L, power_rail_is_enabled());
    return 1;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.audio
// -----------------------------------------------------------------------------
static int l_prop_audio_play(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    bool loop = false;
    if (lua_isboolean(L, 2)) {
        loop = lua_toboolean(L, 2);
    }

    Serial.printf("[LUA] prop.audio.play('%s', loop=%s)\r\n", path, loop ? "true" : "false");

    core_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_AUDIO_PLAY;
    strncpy(cmd.params.audio_play.path, path, MAX_AUDIO_PATH_LEN - 1);
    cmd.params.audio_play.loop = loop;

    ipc_send_command(&cmd);
    return 0;
}

static int l_prop_audio_stop(lua_State* L) {
    Serial.println("[LUA] prop.audio.stop()");
    core_command_t cmd = { CMD_AUDIO_STOP, {} };
    ipc_send_command(&cmd);
    return 0;
}

static int l_prop_audio_pause(lua_State* L) {
    core_command_t cmd = { CMD_AUDIO_PAUSE, {} };
    ipc_send_command(&cmd);
    return 0;
}

static int l_prop_audio_resume(lua_State* L) {
    core_command_t cmd = { CMD_AUDIO_RESUME, {} };
    ipc_send_command(&cmd);
    return 0;
}

static int l_prop_audio_set_volume(lua_State* L) {
    int vol = (int)luaL_checkinteger(L, 1);
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;

    core_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_AUDIO_SET_VOLUME;
    cmd.params.audio_volume.volume = (uint8_t)vol;

    ipc_send_command(&cmd);
    return 0;
}

static int l_prop_audio_tone(lua_State* L) {
    int freq = (int)luaL_checkinteger(L, 1);
    int duration = (int)luaL_optinteger(L, 2, 200); // Default 200 ms
    if (freq < 20) freq = 20;
    if (freq > 20000) freq = 20000;
    if (duration < 1) duration = 1;
    if (duration > 10000) duration = 10000;

    core_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_AUDIO_TONE;
    cmd.params.audio_tone.freq_hz = (uint16_t)freq;
    cmd.params.audio_tone.duration_ms = (uint16_t)duration;

    ipc_send_command(&cmd);
    return 0;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.neopixel
// -----------------------------------------------------------------------------
static int l_prop_neopixel_init(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    int num_pixels = (int)luaL_checkinteger(L, base);
    int pin = PIN_EXTERNAL_NEOPIXELS;
    if (lua_isinteger(L, base + 1)) {
        pin = (int)lua_tointeger(L, base + 1);
    }
    neopixel_init(num_pixels, pin);

    // Return prop.neopixel table for OOP / method chaining
    lua_getglobal(L, "prop");
    lua_getfield(L, -1, "neopixel");
    lua_remove(L, -2);
    return 1;
}

static int l_prop_neopixel_set(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    int idx = (int)luaL_checkinteger(L, base) - 1; // Lua 1-based index to 0-based
    int r = (int)luaL_checkinteger(L, base + 1);
    int g = (int)luaL_checkinteger(L, base + 2);
    int b = (int)luaL_checkinteger(L, base + 3);
    neopixel_set_pixel(idx, r, g, b);
    if (base == 2) {
        lua_pushvalue(L, 1);
        return 1;
    }
    return 0;
}

static int l_prop_neopixel_fill(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    int r = (int)luaL_checkinteger(L, base);
    int g = (int)luaL_checkinteger(L, base + 1);
    int b = (int)luaL_checkinteger(L, base + 2);
    neopixel_fill(r, g, b);
    if (base == 2) {
        lua_pushvalue(L, 1);
        return 1;
    }
    return 0;
}

static int l_prop_neopixel_set_brightness(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    int br = (int)luaL_checkinteger(L, base);
    if (br < 0) br = 0;
    if (br > 255) br = 255;
    neopixel_set_brightness((uint8_t)br);
    if (base == 2) {
        lua_pushvalue(L, 1);
        return 1;
    }
    return 0;
}

static int l_prop_neopixel_show(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    neopixel_show();
    if (base == 2) {
        lua_pushvalue(L, 1);
        return 1;
    }
    return 0;
}

static int l_prop_neopixel_clear(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    neopixel_clear();
    if (base == 2) {
        lua_pushvalue(L, 1);
        return 1;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.status_pixel
// -----------------------------------------------------------------------------
static int l_prop_status_pixel_set(lua_State* L) {
    int r = (int)luaL_checkinteger(L, 1);
    int g = (int)luaL_checkinteger(L, 2);
    int b = (int)luaL_checkinteger(L, 3);
    status_pixel_set_rgb(r, g, b);
    return 0;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.servo
// -----------------------------------------------------------------------------
static int l_prop_servo_init(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    int pin = PIN_EXTERNAL_SERVO;
    int min_us = 500;
    int max_us = 2500;
    if (lua_isinteger(L, base)) pin = (int)lua_tointeger(L, base);
    if (lua_isinteger(L, base + 1)) min_us = (int)lua_tointeger(L, base + 1);
    if (lua_isinteger(L, base + 2)) max_us = (int)lua_tointeger(L, base + 2);

    servo_init(pin, min_us, max_us);

    // Return prop.servo table for OOP / method chaining
    lua_getglobal(L, "prop");
    lua_getfield(L, -1, "servo");
    lua_remove(L, -2);
    return 1;
}

static int l_prop_servo_angle(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    lua_Number deg = luaL_checknumber(L, base);
    servo_write_angle((float)deg);
    if (base == 2) {
        lua_pushvalue(L, 1);
        return 1;
    }
    return 0;
}

static int l_prop_servo_pulse(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    int us = (int)luaL_checkinteger(L, base);
    servo_write_us((uint16_t)us);
    if (base == 2) {
        lua_pushvalue(L, 1);
        return 1;
    }
    return 0;
}

static int l_prop_servo_detach(lua_State* L) {
    int base = (lua_istable(L, 1) || lua_isuserdata(L, 1)) ? 2 : 1;
    servo_detach();
    if (base == 2) {
        lua_pushvalue(L, 1);
        return 1;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Dynamic Button & LED Userdata Structs
// -----------------------------------------------------------------------------
struct LuaPinButton {
    uint8_t pin;
    bool active_low;
};

struct LuaPinLed {
    uint8_t pin;
    bool active_high;
};

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.button
// -----------------------------------------------------------------------------
static int l_prop_button_pressed(lua_State* L) {
    lua_pushboolean(L, button_is_pressed());
    return 1;
}

static int l_prop_button_boot_pressed(lua_State* L) {
    lua_pushboolean(L, boot_button_is_pressed());
    return 1;
}

static int l_pin_btn_pressed(lua_State* L) {
    LuaPinButton* btn = (LuaPinButton*)luaL_checkudata(L, 1, "prop.Button");
    lua_pushboolean(L, pin_button_is_pressed(btn->pin, btn->active_low));
    return 1;
}

static int l_prop_button_new(lua_State* L) {
    int pin = (int)luaL_checkinteger(L, 1);
    const char* pull = luaL_optstring(L, 2, "up");
    bool active_low = true;
    if (lua_isboolean(L, 3)) {
        active_low = lua_toboolean(L, 3);
    }

    uint8_t pull_mode = 0;
    if (strcmp(pull, "down") == 0) pull_mode = 1;
    else if (strcmp(pull, "none") == 0) pull_mode = 2;

    pin_button_init((uint8_t)pin, pull_mode);

    LuaPinButton* btn = (LuaPinButton*)lua_newuserdata(L, sizeof(LuaPinButton));
    btn->pin = (uint8_t)pin;
    btn->active_low = active_low;

    luaL_getmetatable(L, "prop.Button");
    lua_setmetatable(L, -2);
    return 1;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.led
// -----------------------------------------------------------------------------
static int l_prop_led_on(lua_State* L) {
    led_set(true);
    return 0;
}

static int l_prop_led_off(lua_State* L) {
    led_set(false);
    return 0;
}

static int l_prop_led_toggle(lua_State* L) {
    led_toggle();
    return 0;
}

static int l_pin_led_on(lua_State* L) {
    LuaPinLed* led = (LuaPinLed*)luaL_checkudata(L, 1, "prop.Led");
    pin_led_set(led->pin, true, led->active_high);
    return 0;
}

static int l_pin_led_off(lua_State* L) {
    LuaPinLed* led = (LuaPinLed*)luaL_checkudata(L, 1, "prop.Led");
    pin_led_set(led->pin, false, led->active_high);
    return 0;
}

static int l_pin_led_toggle(lua_State* L) {
    LuaPinLed* led = (LuaPinLed*)luaL_checkudata(L, 1, "prop.Led");
    pin_led_toggle(led->pin);
    return 0;
}

static int l_pin_led_pwm(lua_State* L) {
    LuaPinLed* led = (LuaPinLed*)luaL_checkudata(L, 1, "prop.Led");
    int duty = (int)luaL_checkinteger(L, 2);
    if (duty < 0) duty = 0;
    if (duty > 255) duty = 255;
    pin_led_pwm(led->pin, (uint8_t)duty);
    return 0;
}

static int l_prop_led_new(lua_State* L) {
    int pin = (int)luaL_checkinteger(L, 1);
    bool active_high = true;
    if (lua_isboolean(L, 2)) {
        active_high = lua_toboolean(L, 2);
    }
    pin_led_init((uint8_t)pin);

    LuaPinLed* led = (LuaPinLed*)lua_newuserdata(L, sizeof(LuaPinLed));
    led->pin = (uint8_t)pin;
    led->active_high = active_high;

    luaL_getmetatable(L, "prop.Led");
    lua_setmetatable(L, -2);
    return 1;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.gpio
// -----------------------------------------------------------------------------
static int l_prop_gpio_mode(lua_State* L) {
    int pin = (int)luaL_checkinteger(L, 1);
    const char* mode_str = luaL_checkstring(L, 2);
    uint8_t mode = INPUT;
    if (strcmp(mode_str, "output") == 0) mode = OUTPUT;
    else if (strcmp(mode_str, "in_pullup") == 0 || strcmp(mode_str, "input_pullup") == 0) mode = INPUT_PULLUP;
    else if (strcmp(mode_str, "in_pulldown") == 0 || strcmp(mode_str, "input_pulldown") == 0) mode = INPUT_PULLDOWN;
    pin_gpio_mode((uint8_t)pin, mode);
    return 0;
}

static int l_prop_gpio_read(lua_State* L) {
    int pin = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, pin_gpio_read((uint8_t)pin));
    return 1;
}

static int l_prop_gpio_write(lua_State* L) {
    int pin = (int)luaL_checkinteger(L, 1);
    bool val = lua_toboolean(L, 2);
    pin_gpio_write((uint8_t)pin, val);
    return 0;
}

static int l_prop_gpio_pwm(lua_State* L) {
    int pin = (int)luaL_checkinteger(L, 1);
    int duty = (int)luaL_checkinteger(L, 2);
    if (duty < 0) duty = 0;
    if (duty > 255) duty = 255;
    pin_led_pwm((uint8_t)pin, (uint8_t)duty);
    return 0;
}

static int l_prop_gpio_adc(lua_State* L) {
    int pin = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, pin_gpio_adc((uint8_t)pin));
    return 1;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.battery
// -----------------------------------------------------------------------------
static int l_prop_battery_voltage(lua_State* L) {
    lua_pushnumber(L, (lua_Number)battery_read_voltage());
    return 1;
}

static int l_prop_battery_percent(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)battery_read_percent());
    return 1;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.motion
// -----------------------------------------------------------------------------
static int l_prop_motion_read_accel(lua_State* L) {
    float x = 0, y = 0, z = 0;
    if (motion_read_accel(&x, &y, &z)) {
        lua_pushnumber(L, (lua_Number)x);
        lua_pushnumber(L, (lua_Number)y);
        lua_pushnumber(L, (lua_Number)z);
        return 3;
    }
    return 0;
}

static int l_prop_motion_is_tapped(lua_State* L) {
    lua_pushboolean(L, motion_is_tapped());
    return 1;
}

// -----------------------------------------------------------------------------
// Lua C API Bindings: prop.time
// -----------------------------------------------------------------------------
static int l_prop_time_sleep_ms(lua_State* L) {
    int ms = (int)luaL_checkinteger(L, 1);
    if (ms > 0) {
        lua_engine_yield_sleep((uint32_t)ms);
    }
    return 0;
}

static int l_prop_time_ticks_ms(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)millis());
    return 1;
}

// -----------------------------------------------------------------------------
// Register prop.* namespace in Lua
// -----------------------------------------------------------------------------
static void register_prop_api(lua_State* L) {
    lua_newtable(L); // prop table

    // prop.power
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_power_enable); lua_setfield(L, -2, "enable");
    lua_pushcfunction(L, l_prop_power_disable); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, l_prop_power_is_enabled); lua_setfield(L, -2, "is_enabled");
    lua_setfield(L, -2, "power");

    // prop.audio
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_audio_play); lua_setfield(L, -2, "play");
    lua_pushcfunction(L, l_prop_audio_stop); lua_setfield(L, -2, "stop");
    lua_pushcfunction(L, l_prop_audio_pause); lua_setfield(L, -2, "pause");
    lua_pushcfunction(L, l_prop_audio_resume); lua_setfield(L, -2, "resume");
    lua_pushcfunction(L, l_prop_audio_set_volume); lua_setfield(L, -2, "set_volume");
    lua_pushcfunction(L, l_prop_audio_tone); lua_setfield(L, -2, "tone");
    lua_setfield(L, -2, "audio");

    // prop.neopixel
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_neopixel_init); lua_setfield(L, -2, "init");
    lua_pushcfunction(L, l_prop_neopixel_set); lua_setfield(L, -2, "set");
    lua_pushcfunction(L, l_prop_neopixel_fill); lua_setfield(L, -2, "fill");
    lua_pushcfunction(L, l_prop_neopixel_set_brightness); lua_setfield(L, -2, "set_brightness");
    lua_pushcfunction(L, l_prop_neopixel_show); lua_setfield(L, -2, "show");
    lua_pushcfunction(L, l_prop_neopixel_clear); lua_setfield(L, -2, "clear");
    lua_setfield(L, -2, "neopixel");

    // prop.status_pixel
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_status_pixel_set); lua_setfield(L, -2, "set");
    lua_setfield(L, -2, "status_pixel");

    // prop.servo
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_servo_init); lua_setfield(L, -2, "init");
    lua_pushcfunction(L, l_prop_servo_angle); lua_setfield(L, -2, "angle");
    lua_pushcfunction(L, l_prop_servo_pulse); lua_setfield(L, -2, "pulse");
    lua_pushcfunction(L, l_prop_servo_detach); lua_setfield(L, -2, "detach");
    lua_setfield(L, -2, "servo");

    // Register prop.Button metatable
    luaL_newmetatable(L, "prop.Button");
    lua_newtable(L);
    lua_pushcfunction(L, l_pin_btn_pressed); lua_setfield(L, -2, "pressed");
    lua_pushcfunction(L, l_pin_btn_pressed); lua_setfield(L, -2, "read");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    // Register prop.Led metatable
    luaL_newmetatable(L, "prop.Led");
    lua_newtable(L);
    lua_pushcfunction(L, l_pin_led_on); lua_setfield(L, -2, "on");
    lua_pushcfunction(L, l_pin_led_off); lua_setfield(L, -2, "off");
    lua_pushcfunction(L, l_pin_led_toggle); lua_setfield(L, -2, "toggle");
    lua_pushcfunction(L, l_pin_led_pwm); lua_setfield(L, -2, "pwm");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    // prop.button
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_button_new); lua_setfield(L, -2, "new");
    lua_pushcfunction(L, l_prop_button_pressed); lua_setfield(L, -2, "pressed");
    lua_pushcfunction(L, l_prop_button_boot_pressed); lua_setfield(L, -2, "boot_pressed");
    lua_setfield(L, -2, "button");

    // prop.led
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_led_new); lua_setfield(L, -2, "new");
    lua_pushcfunction(L, l_prop_led_on); lua_setfield(L, -2, "on");
    lua_pushcfunction(L, l_prop_led_off); lua_setfield(L, -2, "off");
    lua_pushcfunction(L, l_prop_led_toggle); lua_setfield(L, -2, "toggle");
    lua_setfield(L, -2, "led");

    // prop.gpio
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_gpio_mode); lua_setfield(L, -2, "mode");
    lua_pushcfunction(L, l_prop_gpio_read); lua_setfield(L, -2, "read");
    lua_pushcfunction(L, l_prop_gpio_write); lua_setfield(L, -2, "write");
    lua_pushcfunction(L, l_prop_gpio_pwm); lua_setfield(L, -2, "pwm");
    lua_pushcfunction(L, l_prop_gpio_adc); lua_setfield(L, -2, "adc");
    lua_setfield(L, -2, "gpio");

    // prop.battery
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_battery_voltage); lua_setfield(L, -2, "voltage");
    lua_pushcfunction(L, l_prop_battery_percent); lua_setfield(L, -2, "percent");
    lua_setfield(L, -2, "battery");

    // prop.motion
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_motion_read_accel); lua_setfield(L, -2, "read_accel");
    lua_pushcfunction(L, l_prop_motion_is_tapped); lua_setfield(L, -2, "is_tapped");
    lua_setfield(L, -2, "motion");

    // prop.time
    lua_newtable(L);
    lua_pushcfunction(L, l_prop_time_sleep_ms); lua_setfield(L, -2, "sleep_ms");
    lua_pushcfunction(L, l_prop_time_ticks_ms); lua_setfield(L, -2, "ticks_ms");
    lua_setfield(L, -2, "time");

    // Register globally as 'prop'
    lua_setglobal(L, "prop");
}

static int l_prop_print(lua_State* L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        if (i > 1) Serial.print("\t");
        if (s) Serial.print(s);
        lua_pop(L, 1);
    }
    Serial.println();
    return 0;
}

// -----------------------------------------------------------------------------
// FatFS Reader Callback for lua_load
// -----------------------------------------------------------------------------
struct FileReaderState {
    File f;
    char buffer[256];
};

static const char* fatfs_lua_reader(lua_State* L, void* data, size_t* size) {
    (void)L;
    FileReaderState* state = (FileReaderState*)data;
    if (state->f.available()) {
        *size = state->f.readBytes(state->buffer, sizeof(state->buffer));
        return state->buffer;
    }
    *size = 0;
    return nullptr;
}

// -----------------------------------------------------------------------------
// Lua Engine Lifecycle
// -----------------------------------------------------------------------------
bool lua_engine_init(void) {
    lua_engine_stop();

    L = luaL_newstate();
    if (!L) return false;

    // Load standard Lua libraries
    luaL_openlibs(L);

    // Register prop.* bindings
    register_prop_api(L);

    // Route global print to USB CDC Serial
    lua_register(L, "print", l_prop_print);

    // Install cooperative execution hook every 1000 VM instructions
    lua_sethook(L, lua_count_hook, LUA_MASKCOUNT, 1000);

    has_error = false;
    error_msg[0] = '\0';
    return true;
}

void lua_engine_stop(void) {
    if (L) {
        lua_close(L);
        L = nullptr;
    }
}

bool lua_engine_run_file(const char* filepath) {
    if (!lua_engine_init()) return false;

    File f = FatFS.open(filepath, "r");
    if (!f) {
        has_error = true;
        snprintf(error_msg, sizeof(error_msg), "Could not open script file: %s", filepath);
        Serial.printf("[LUA ERROR] %s\r\n", error_msg);
        return false;
    }

    FileReaderState reader_state;
    reader_state.f = f;

    // Load file chunk
    int status = lua_load(L, fatfs_lua_reader, &reader_state, filepath, nullptr);
    f.close();

    if (status != LUA_OK) {
        has_error = true;
        const char* err = lua_tostring(L, -1);
        strncpy(error_msg, err ? err : "Syntax error loading file", sizeof(error_msg) - 1);
        lua_pop(L, 1);
        Serial.printf("[LUA SYNTAX ERROR] %s\r\n", error_msg);
        return false;
    }

    // Execute chunk with protected call
    status = lua_pcall(L, 0, 0, 0);
    if (status != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        if (err && strstr(err, "__PROP_RELOAD__")) {
            // Clean exit triggered by file write / reload request
            lua_pop(L, 1);
            has_error = false;
            return false;
        }
        has_error = true;
        strncpy(error_msg, err ? err : "Runtime error executing script", sizeof(error_msg) - 1);
        lua_pop(L, 1);
        Serial.printf("[LUA RUNTIME ERROR] %s\r\n", error_msg);
        return false;
    }

    return true;
}

bool lua_engine_has_error(void) {
    return has_error;
}

const char* lua_engine_get_error(void) {
    return error_msg;
}

void lua_engine_clear_error(void) {
    has_error = false;
    error_msg[0] = '\0';
}
