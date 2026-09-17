#include <unity.h>
#include <string.h>

#include "mock_hardware.h"
#include "board_config.h"
#include "ipc_protocol.h"
#include "peripheral_mgr.h"
#include "lua_engine.h"
#include "FatFS.h"
#include "Adafruit_LIS3DH.h"

void setUp(void) {
    mock_hardware_reset();
    peripherals_teardown();
    lua_engine_stop();
    lua_engine_clear_error();
}

void tearDown(void) {
    peripherals_teardown();
    lua_engine_stop();
}

// -----------------------------------------------------------------------------
// 1. IPC Protocol Tests
// -----------------------------------------------------------------------------

void test_ipc_command_queue_basic(void) {
    ipc_init();

    core_command_t cmd_out;
    memset(&cmd_out, 0, sizeof(cmd_out));
    cmd_out.type = CMD_AUDIO_PLAY;
    strncpy(cmd_out.params.audio_play.path, "/test.wav", sizeof(cmd_out.params.audio_play.path) - 1);
    cmd_out.params.audio_play.loop = false;

    bool sent = ipc_send_command(&cmd_out);
    TEST_ASSERT_TRUE(sent);

    core_command_t cmd_in;
    memset(&cmd_in, 0, sizeof(cmd_in));
    bool recvd = ipc_receive_command(&cmd_in);
    TEST_ASSERT_TRUE(recvd);
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_PLAY, cmd_in.type);
    TEST_ASSERT_EQUAL_STRING("/test.wav", cmd_in.params.audio_play.path);
    TEST_ASSERT_FALSE(cmd_in.params.audio_play.loop);

    // Queue should now be empty
    TEST_ASSERT_FALSE(ipc_receive_command(&cmd_in));
}

void test_ipc_command_queue_ordering(void) {
    ipc_init();

    core_command_t cmd1 = { CMD_AUDIO_PLAY, {} };
    core_command_t cmd2 = { CMD_AUDIO_STOP, {} };
    core_command_t cmd3 = { CMD_SERVO_DETACH, {} };

    TEST_ASSERT_TRUE(ipc_send_command(&cmd1));
    TEST_ASSERT_TRUE(ipc_send_command(&cmd2));
    TEST_ASSERT_TRUE(ipc_send_command(&cmd3));

    core_command_t out;
    TEST_ASSERT_TRUE(ipc_receive_command(&out));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_PLAY, out.type);

    TEST_ASSERT_TRUE(ipc_receive_command(&out));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_STOP, out.type);

    TEST_ASSERT_TRUE(ipc_receive_command(&out));
    TEST_ASSERT_EQUAL_INT(CMD_SERVO_DETACH, out.type);

    TEST_ASSERT_FALSE(ipc_receive_command(&out));
}

void test_ipc_event_queue_and_logging(void) {
    ipc_init();

    ipc_log("Test log %d: %s", 42, "hello");

    core_event_t evt;
    TEST_ASSERT_TRUE(ipc_receive_event(&evt));
    TEST_ASSERT_EQUAL_INT(EVT_DEBUG_LOG, evt.type);
    TEST_ASSERT_EQUAL_STRING("Test log 42: hello", evt.text);

    TEST_ASSERT_FALSE(ipc_receive_event(&evt));
}

// -----------------------------------------------------------------------------
// 2. Peripheral Manager Tests
// -----------------------------------------------------------------------------

void test_peripheral_power_rail(void) {
    power_rail_disable();
    TEST_ASSERT_FALSE(power_rail_is_enabled());
    TEST_ASSERT_EQUAL_INT(LOW, mock_get_digital_pin(PIN_EXTERNAL_POWER));

    power_rail_enable();
    TEST_ASSERT_TRUE(power_rail_is_enabled());
    TEST_ASSERT_EQUAL_INT(HIGH, mock_get_digital_pin(PIN_EXTERNAL_POWER));

    power_rail_disable();
    TEST_ASSERT_FALSE(power_rail_is_enabled());
    TEST_ASSERT_EQUAL_INT(LOW, mock_get_digital_pin(PIN_EXTERNAL_POWER));
}

void test_peripheral_led(void) {
    led_set(true);
    TEST_ASSERT_EQUAL_INT(HIGH, mock_get_digital_pin(PIN_ONBOARD_LED));

    led_set(false);
    TEST_ASSERT_EQUAL_INT(LOW, mock_get_digital_pin(PIN_ONBOARD_LED));

    led_toggle();
    TEST_ASSERT_EQUAL_INT(HIGH, mock_get_digital_pin(PIN_ONBOARD_LED));

    led_toggle();
    TEST_ASSERT_EQUAL_INT(LOW, mock_get_digital_pin(PIN_ONBOARD_LED));
}

void test_peripheral_neopixel(void) {
    bool ok = neopixel_init(8, PIN_EXTERNAL_NEOPIXELS);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(power_rail_is_enabled()); // Power rail auto-enabled for NeoPixels

    neopixel_fill(10, 20, 30);
    neopixel_set_pixel(0, 255, 0, 0);
    neopixel_set_brightness(128);
    neopixel_show();
    neopixel_clear();
}

void test_peripheral_servo(void) {
    bool ok = servo_init(PIN_EXTERNAL_SERVO, 500, 2500);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(power_rail_is_enabled()); // Power rail auto-enabled for Servo

    servo_write_angle(90.0f);
    servo_write_us(1800);
    servo_detach();
}

void test_peripheral_buttons(void) {
    // Buttons are active low (with pull-up)
    mock_set_digital_pin(PIN_EXTERNAL_BUTTON, HIGH);
    TEST_ASSERT_FALSE(button_is_pressed());

    mock_set_digital_pin(PIN_EXTERNAL_BUTTON, LOW);
    TEST_ASSERT_TRUE(button_is_pressed());

    mock_set_digital_pin(PIN_BOOT_BUTTON, HIGH);
    TEST_ASSERT_FALSE(boot_button_is_pressed());

    mock_set_digital_pin(PIN_BOOT_BUTTON, LOW);
    TEST_ASSERT_TRUE(boot_button_is_pressed());
}

void test_peripheral_battery(void) {
    // 12-bit ADC (0..4095), 3.3V ref, 2x divider
    // At full 4095: (4095/4095) * 3.3 * 2 = 6.6V
    // For 3.7V: raw = (3.7 / 6.6) * 4095 = 2295
    mock_set_analog_pin(PIN_BATTERY_ADC, 2295);
    float v = battery_read_voltage();
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 3.70f, v);

    uint8_t pct = battery_read_percent();
    // 3.7V is 50% between 3.2V and 4.2V
    TEST_ASSERT_INT_WITHIN(5, 50, pct);
}

void test_peripheral_motion(void) {
    bool ok = motion_init();
    TEST_ASSERT_TRUE(ok);

    Adafruit_LIS3DH::mock_set_accel(0.1f, -0.2f, 0.98f);
    float x = 0, y = 0, z = 0;
    TEST_ASSERT_TRUE(motion_read_accel(&x, &y, &z));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -0.2f, y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.98f, z);

    Adafruit_LIS3DH::mock_set_click(0x00);
    TEST_ASSERT_FALSE(motion_is_tapped());

    Adafruit_LIS3DH::mock_set_click(0x10); // Bit 4 set
    TEST_ASSERT_TRUE(motion_is_tapped());
}

void test_peripheral_dynamic_pins(void) {
    const uint8_t test_pin = 10;

    pin_led_init(test_pin);
    TEST_ASSERT_EQUAL_INT(OUTPUT, mock_get_pin_mode(test_pin));

    pin_led_set(test_pin, true, true);
    TEST_ASSERT_EQUAL_INT(HIGH, mock_get_digital_pin(test_pin));

    pin_led_set(test_pin, false, true);
    TEST_ASSERT_EQUAL_INT(LOW, mock_get_digital_pin(test_pin));

    pin_led_toggle(test_pin);
    TEST_ASSERT_EQUAL_INT(HIGH, mock_get_digital_pin(test_pin));

    pin_button_init(test_pin, 0); // Pullup
    TEST_ASSERT_EQUAL_INT(INPUT_PULLUP, mock_get_pin_mode(test_pin));

    mock_set_digital_pin(test_pin, LOW);
    TEST_ASSERT_TRUE(pin_button_is_pressed(test_pin, true));

    mock_set_digital_pin(test_pin, HIGH);
    TEST_ASSERT_FALSE(pin_button_is_pressed(test_pin, true));
}

void test_peripheral_teardown(void) {
    power_rail_enable();
    led_set(true);
    pin_led_init(12);
    pin_led_set(12, true, true);

    TEST_ASSERT_TRUE(power_rail_is_enabled());
    TEST_ASSERT_EQUAL_INT(HIGH, mock_get_digital_pin(PIN_ONBOARD_LED));

    peripherals_teardown();

    TEST_ASSERT_FALSE(power_rail_is_enabled());
    TEST_ASSERT_EQUAL_INT(LOW, mock_get_digital_pin(PIN_ONBOARD_LED));
    TEST_ASSERT_EQUAL_INT(INPUT, mock_get_pin_mode(12));
}

// -----------------------------------------------------------------------------
// 3. Lua Engine Tests
// -----------------------------------------------------------------------------

void test_lua_lifecycle_and_execution(void) {
    ipc_init();
    TEST_ASSERT_TRUE(lua_engine_init());
    TEST_ASSERT_FALSE(lua_engine_has_error());

    // Run simple valid script
    FatFS.set_file("/code.lua", "local x = 1 + 2; prop.led.on()");
    bool ok = lua_engine_run_file("/code.lua");
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FALSE(lua_engine_has_error());
    TEST_ASSERT_EQUAL_INT(HIGH, mock_get_digital_pin(PIN_ONBOARD_LED));

    lua_engine_stop();
}

void test_lua_syntax_error(void) {
    ipc_init();
    FatFS.set_file("/bad.lua", "this is totally invalid lua syntax !!!");
    bool ok = lua_engine_run_file("/bad.lua");
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(lua_engine_has_error());
    TEST_ASSERT_NOT_NULL(strstr(lua_engine_get_error(), "syntax"));

    lua_engine_clear_error();
    TEST_ASSERT_FALSE(lua_engine_has_error());
}

void test_lua_runtime_error(void) {
    ipc_init();
    FatFS.set_file("/runtime.lua", "local a = nil; a.foo = 42");
    bool ok = lua_engine_run_file("/runtime.lua");
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(lua_engine_has_error());

    lua_engine_clear_error();
}

void test_lua_audio_bindings(void) {
    ipc_init();

    FatFS.set_file("/audio.lua", 
        "prop.audio.play('blaster.wav', false)\n"
        "prop.audio.volume(80)\n"
        "prop.audio.tone(440, 150)\n"
        "prop.audio.pause()\n"
        "prop.audio.resume()\n"
        "prop.audio.stop()\n"
    );

    TEST_ASSERT_TRUE(lua_engine_run_file("/audio.lua"));

    core_command_t cmd;
    // 1. play
    TEST_ASSERT_TRUE(ipc_receive_command(&cmd));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_PLAY, cmd.type);
    TEST_ASSERT_EQUAL_STRING("blaster.wav", cmd.params.audio_play.path);
    TEST_ASSERT_FALSE(cmd.params.audio_play.loop);

    // 2. volume
    TEST_ASSERT_TRUE(ipc_receive_command(&cmd));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_SET_VOLUME, cmd.type);
    TEST_ASSERT_EQUAL_UINT8(80, cmd.params.audio_volume.volume);

    // 3. tone
    TEST_ASSERT_TRUE(ipc_receive_command(&cmd));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_TONE, cmd.type);
    TEST_ASSERT_EQUAL_UINT16(440, cmd.params.audio_tone.freq_hz);
    TEST_ASSERT_EQUAL_UINT16(150, cmd.params.audio_tone.duration_ms);

    // 4. pause
    TEST_ASSERT_TRUE(ipc_receive_command(&cmd));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_PAUSE, cmd.type);

    // 5. resume
    TEST_ASSERT_TRUE(ipc_receive_command(&cmd));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_RESUME, cmd.type);

    // 6. stop
    TEST_ASSERT_TRUE(ipc_receive_command(&cmd));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_STOP, cmd.type);

    TEST_ASSERT_FALSE(ipc_receive_command(&cmd));
}

void test_lua_audio_is_playing(void) {
    ipc_init();

    mock_set_audio_playing(false);
    FatFS.set_file("/is_playing.lua", 
        "if prop.audio.is_playing() then\n"
        "    prop.led.on()\n"
        "else\n"
        "    prop.led.off()\n"
        "end\n"
    );

    TEST_ASSERT_TRUE(lua_engine_run_file("/is_playing.lua"));
    TEST_ASSERT_EQUAL_INT(LOW, mock_get_digital_pin(PIN_ONBOARD_LED));

    mock_set_audio_playing(true);
    TEST_ASSERT_TRUE(lua_engine_run_file("/is_playing.lua"));
    TEST_ASSERT_EQUAL_INT(HIGH, mock_get_digital_pin(PIN_ONBOARD_LED));
}

void test_lua_power_and_peripherals(void) {
    ipc_init();

    FatFS.set_file("/periph.lua",
        "prop.power.enable()\n"
        "prop.servo.init(20, 500, 2500)\n"
        "prop.servo.angle(45)\n"
        "prop.neopixel.init(10, 21)\n"
        "prop.neopixel.fill(255, 128, 64)\n"
        "prop.neopixel.show()\n"
    );

    TEST_ASSERT_TRUE(lua_engine_run_file("/periph.lua"));
    TEST_ASSERT_TRUE(power_rail_is_enabled());
}

void test_lua_status_pixel(void) {
    ipc_init();

    FatFS.set_file("/pixel.lua",
        "prop.status_pixel.set(255, 0, 128)\n"
        "prop.status_pixel.set_brightness(100)\n"
        "prop.status_pixel.off()\n"
        "prop.pixel.rgb(10, 20, 30)\n"
        "prop.pixel.brightness(50)\n"
        "prop.pixel.clear()\n"
    );

    TEST_ASSERT_TRUE(lua_engine_run_file("/pixel.lua"));
    TEST_ASSERT_FALSE(lua_engine_has_error());
}

// -----------------------------------------------------------------------------
// Test Runner Entrypoint
// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // IPC Protocol
    RUN_TEST(test_ipc_command_queue_basic);
    RUN_TEST(test_ipc_command_queue_ordering);
    RUN_TEST(test_ipc_event_queue_and_logging);

    // Peripheral Manager
    RUN_TEST(test_peripheral_power_rail);
    RUN_TEST(test_peripheral_led);
    RUN_TEST(test_peripheral_neopixel);
    RUN_TEST(test_peripheral_servo);
    RUN_TEST(test_peripheral_buttons);
    RUN_TEST(test_peripheral_battery);
    RUN_TEST(test_peripheral_motion);
    RUN_TEST(test_peripheral_dynamic_pins);
    RUN_TEST(test_peripheral_teardown);

    // Lua Engine
    RUN_TEST(test_lua_lifecycle_and_execution);
    RUN_TEST(test_lua_syntax_error);
    RUN_TEST(test_lua_runtime_error);
    RUN_TEST(test_lua_audio_bindings);
    RUN_TEST(test_lua_audio_is_playing);
    RUN_TEST(test_lua_power_and_peripherals);
    RUN_TEST(test_lua_status_pixel);

    return UNITY_END();
}
