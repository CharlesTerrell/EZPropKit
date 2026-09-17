#include <Arduino.h>
#include <unity.h>

#include "board_config.h"
#include "ipc_protocol.h"
#include "peripheral_mgr.h"
#include "lua_engine.h"

void setUp(void) {
    peripherals_teardown();
}

void tearDown(void) {
    peripherals_teardown();
}

void test_embedded_system_clock(void) {
    // Verify 200 MHz overclock is applied
    TEST_ASSERT_EQUAL_UINT32(200000000UL, F_CPU);
}

void test_embedded_ipc_command_queue(void) {
    ipc_init();

    core_command_t cmd_out;
    memset(&cmd_out, 0, sizeof(cmd_out));
    cmd_out.type = CMD_AUDIO_STOP;

    TEST_ASSERT_TRUE(ipc_send_command(&cmd_out));

    core_command_t cmd_in;
    memset(&cmd_in, 0, sizeof(cmd_in));
    TEST_ASSERT_TRUE(ipc_receive_command(&cmd_in));
    TEST_ASSERT_EQUAL_INT(CMD_AUDIO_STOP, cmd_in.type);
}

void test_embedded_power_rail(void) {
    power_rail_disable();
    TEST_ASSERT_FALSE(power_rail_is_enabled());

    power_rail_enable();
    TEST_ASSERT_TRUE(power_rail_is_enabled());

    power_rail_disable();
    TEST_ASSERT_FALSE(power_rail_is_enabled());
}

void test_embedded_led_control(void) {
    led_set(true);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PIN_ONBOARD_LED));

    led_set(false);
    TEST_ASSERT_EQUAL(LOW, digitalRead(PIN_ONBOARD_LED));
}

void test_embedded_lua_lifecycle(void) {
    bool ok = lua_engine_init();
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FALSE(lua_engine_has_error());
    lua_engine_stop();
}

void setup() {
    // Wait for USB CDC Serial monitor to connect
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0 < 3000)) {
        delay(10);
    }
    delay(500);

    UNITY_BEGIN();
    RUN_TEST(test_embedded_system_clock);
    RUN_TEST(test_embedded_ipc_command_queue);
    RUN_TEST(test_embedded_power_rail);
    RUN_TEST(test_embedded_led_control);
    RUN_TEST(test_embedded_lua_lifecycle);
    UNITY_END();
}

void loop() {
    delay(100);
}
