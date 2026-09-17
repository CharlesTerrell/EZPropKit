#include <Arduino.h>
#include "board_config.h"
#include "ipc_protocol.h"
#include "peripheral_mgr.h"
#include "core1_engine.h"
#include "msc_disk.h"
#include "lua_engine.h"

#include <FatFS.h>

static char active_script[64] = {0};
static bool script_running = false;
static bool script_reload_pending = false;

static void reload_script(void) {
    Serial.println("\r\n================================================");
    Serial.println("[SUPERVISOR] Detected file changes on EZPROPKIT.");
    Serial.println("[SUPERVISOR] Resetting peripherals & reloading Lua...");
    Serial.println("================================================\r\n");

    // 1. Stop existing Lua VM
    lua_engine_stop();
    lua_engine_clear_error();
    script_running = false;

    // 2. Reset Core 1 peripherals (cut audio) and wait until audio file is closed
    core_command_t cmd = { CMD_RESET_PERIPHERALS, {} };
    ipc_send_command(&cmd);

    uint32_t t0 = millis();
    while (core1_audio_is_playing() && (millis() - t0 < 200)) {
        delay(5);
    }

    // 3. Reset Core 0 peripherals (detach servo, turn off NeoPixels, reset GPIOs)
    peripherals_teardown();

    // 4. File handles are closed; newly written files are immediately readable via FTL.

    // 5. Find and run script
    if (msc_disk_find_script(active_script, sizeof(active_script))) {
        Serial.printf("[SUPERVISOR] Executing: %s\r\n", active_script);
        // Set status LED to gentle green on successful boot
        status_pixel_set_rgb(0, 40, 0);
        script_running = lua_engine_run_file(active_script);
    } else {
        Serial.println("[SUPERVISOR] Awaiting code.lua or main.lua on drive...");
        // Blue pulsing beacon while waiting for code
        status_pixel_set_rgb(0, 0, 40);
    }
}

static uint32_t boot_stabilize_until_ms = 0;

#ifndef UNIT_TEST
void setup() {
    // 0. Immediately hold 5V boost converter OFF and clamp I2S lines LOW
    // to put MAX98357A in hardware shutdown and prevent startup static/crackle
    pinMode(PIN_EXTERNAL_POWER, OUTPUT);
    digitalWrite(PIN_EXTERNAL_POWER, LOW);
    pinMode(PIN_I2S_BCLK, OUTPUT);
    digitalWrite(PIN_I2S_BCLK, LOW);
    pinMode(PIN_I2S_DATA, OUTPUT);
    digitalWrite(PIN_I2S_DATA, LOW);
    pinMode(PIN_I2S_LRCLK, OUTPUT);
    digitalWrite(PIN_I2S_LRCLK, LOW);

    // 2. Initialize USB Serial Console
    Serial.begin(115200);

    // 3. Initialize red LED & status pixel
    led_set(true);
    status_pixel_set_rgb(20, 20, 20); // White startup flash

    // 4. Initialize Inter-Core IPC
    ipc_init();

    // 5. Initialize USB Mass Storage & FatFS on flash
    bool disk_ok = msc_disk_init();

    delay(200);
    led_set(false);

    Serial.println("\r\n+----------------------------------------------------+");
    Serial.println("| ezpropkit: Lua & C Engine for Prop-Maker Feather   |");
    Serial.println("| Target: RP2040 @ 200 MHz                           |");
    Serial.println("| USB Volume: EZPROPKIT                              |");
    Serial.println("+----------------------------------------------------+");

    if (!disk_ok) {
        Serial.println("[ERROR] Failed to mount SPI flash filesystem!");
        Serial.println("[SUPERVISOR] Awaiting filesystem initialization or format from PC...");
        status_pixel_set_rgb(80, 0, 0); // Red error
    }

    // 6. Schedule initial script load with settling window for USB enumeration
    boot_stabilize_until_ms = millis() + 2000;
    script_reload_pending = true;
}

void loop() {
    // 1. Process USB background tasks
    yield();

    // 2. Process file write watcher
    msc_disk_task();

    // 3. If host is currently writing to the drive, halt script & audio immediately
    if (msc_disk_is_writing()) {
        if (script_running) {
            Serial.println("[SUPERVISOR] Host file write in progress. Halting script & audio...");
            lua_engine_stop();
            script_running = false;
            core_command_t cmd = { CMD_RESET_PERIPHERALS, {} };
            ipc_send_command(&cmd);
            peripherals_teardown();
            status_pixel_set_rgb(50, 25, 0); // Amber write indicator
        }
        script_reload_pending = false;
    }

    // 4. Check if auto-reload was triggered by host file save or boot (after USB settles)
    if (msc_disk_check_reload() || (script_reload_pending && millis() >= boot_stabilize_until_ms)) {
        script_reload_pending = false;
        reload_script();
    }

    // 4. If Lua script threw an error, pulse the breathing error beacon
    if (lua_engine_has_error()) {
        status_pixel_pulse_error();
    }

    // 5. Process any telemetry events from Core 1
    lua_engine_pump_ipc();

    delay(2);
}
#endif

