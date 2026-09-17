# Testing Guide for EZPropKit

This document outlines the testing architecture, mock framework, test suites, and workflows for **EZPropKit**. It is intended to help developers and new contributors understand how our tests work, how to run them, and how to write new tests.

---

## 1. Testing Architecture Overview

EZPropKit employs a **two-tier testing strategy** powered by PlatformIO and the [Unity](https://github.com/ThrowTheSwitch/Unity) test framework:

```
                  +----------------------------------------------+
                  |               EZPropKit Testing              |
                  +----------------------------------------------+
                                         |
                 +-----------------------+-----------------------+
                 |                                               |
                 v                                               v
   +---------------------------+                   +---------------------------+
   |    Host-Native Testing    |                   |    On-Target Embedded     |
   |    `[env:native]`         |                   |    `[env:feather_rp2040]` |
   +---------------------------+                   +---------------------------+
   | * Runs on Linux / macOS   |                   | * Runs on RP2040 board    |
   | * No physical hardware    |                   | * Real hardware & GPIOs   |
   | * Full suite in < 1 sec   |                   | * Real 200 MHz overclock  |
   | * Mocks Arduino & Pico    |                   | * Verifies true firmware  |
   | * Ideal for CI/CD & TDD   |                   | * Serial output to host   |
   +---------------------------+                   +---------------------------+
```

### Why a Dual-Tier Strategy?
* **Firmware development is traditionally slow** if every change requires flashing physical microcontrollers over USB.
* **Host-native tests** allow rapid test-driven development (TDD), instantaneous regression testing, and automated headless validation in GitHub Actions / Docker without physical test rigs.
* **On-target embedded tests** guarantee that the actual compiled ARM binary, clocks, DMA channels, and physical electronic rails behave as expected on real silicon.

---

## 2. Host-Native Testing (`[env:native]`)

### How Host-Native Testing Works
Host-native testing compiles the core C++ firmware logic directly on your local workstation using your host compiler (`gcc` or `clang`) targeting x86_64. 

#### Isolation Boundaries & Source Filtering
In [`platformio.ini`](platformio.ini), the `[env:native]` environment selectively compiles the hardware-independent engine modules while omitting RP2040 register-level and flash drivers:

* **Compiled Natively**:
  * [`src/ipc_protocol.cpp`](src/ipc_protocol.cpp): Inter-core FIFO queue protocol and telemetry logging.
  * [`src/peripheral_mgr.cpp`](src/peripheral_mgr.cpp): Power rail sequencing, pin configuration, LED control, battery voltage math, servo pulses, and teardown logic.
  * [`src/lua_engine.cpp`](src/lua_engine.cpp): Lua 5.4 VM lifecycle, script loader, syntax/runtime error handlers, and all `prop.*` bindings.
* **Excluded from Native Build**:
  * `src/core1_engine.cpp`: Excluded because it directly addresses Earle Philhower `BackgroundAudio` I2S DMA registers and PIO assembly.
  * `src/msc_disk.cpp`: Excluded because it interacts with TinyUSB SCSI endpoints and onboard SPI flash hardware.
  * `src/main.cpp`: Excluded from the test executable via `build_src_filter = -<main.cpp>` (and protected by `#ifndef UNIT_TEST`) to prevent entrypoint conflicts with the Unity test runner.

#### The Mock Hardware Layer (`test/test_native/mocks/`)
To allow firmware sources to compile cleanly without Arduino or Raspberry Pi Pico SDK toolchains, we created a lightweight mock layer:

| Mock Header | Purpose | Key Simulated Features |
| :--- | :--- | :--- |
| [`Arduino.h`](test/test_native/mocks/Arduino.h) | Arduino Core APIs | Virtual GPIO pin state array (`pinMode`, `digitalWrite`, `digitalRead`, `analogRead`, `analogWrite`), timebase simulation (`millis`, `delay`), and USB `Serial` emulation with `printf`. |
| [`pico/util/queue.h`](test/test_native/mocks/pico/util/queue.h) | Pico SDK IPC | In-memory circular FIFO ring buffer duplicating Raspberry Pi Pico SDK's `queue_t` API (`queue_init`, `queue_try_add`, `queue_try_remove`). |
| [`FatFS.h`](test/test_native/mocks/FatFS.h) | Virtual Filesystem | In-memory file dictionary (`MockFatFS`, `File`) allowing tests to mount virtual scripts (`/code.lua`) without physical flash chips. |
| [`Adafruit_NeoPixel.h`](test/test_native/mocks/Adafruit_NeoPixel.h) | WS2812B LEDs | In-memory RGB buffer tracking pixel colors, brightness scaling, and buffer flushes. |
| [`Adafruit_LIS3DH.h`](test/test_native/mocks/Adafruit_LIS3DH.h) & [`Wire.h`](test/test_native/mocks/Wire.h) | Accelerometer & I2C | Virtual I2C bus and 3-axis motion sensor supporting synthetic acceleration injection (`mock_set_accel`) and tap gestures (`mock_set_click`). |
| [`Servo.h`](test/test_native/mocks/Servo.h) | PWM Hobby Servo | Tracks servo attachment status, commanded angles (0–180°), and pulse widths (500–2500 µs). |
| [`mock_hardware.h`](test/test_native/mock_hardware.h) / [`.cpp`](test/test_native/mock_hardware.cpp) | Test Controller | Test harness hooks to reset hardware state between tests (`mock_hardware_reset`), advance time (`mock_advance_millis`), and simulate Core 1 audio playback states (`mock_set_audio_playing`). |

---

## 3. Test Suites & Performed Checks

### Native Unit Tests ([`test/test_native/test_main.cpp`](test/test_native/test_main.cpp))

The native test runner contains 19 automated test cases divided across three core subsystems:

#### A. Inter-Core IPC Protocol
* `test_ipc_command_queue_basic`: Verifies queue initialization, single-command pushing (`CMD_AUDIO_PLAY`), parameter extraction, and empty-queue detection.
* `test_ipc_command_queue_ordering`: Ensures strict FIFO ordering across multiple queued commands (`CMD_AUDIO_PLAY` $\rightarrow$ `CMD_AUDIO_STOP` $\rightarrow$ `CMD_SERVO_DETACH`).
* `test_ipc_event_queue_and_logging`: Verifies formatted debug logging via `ipc_log(...)` and event reception.

#### B. Peripheral Manager
* `test_peripheral_power_rail`: Verifies that `power_rail_enable()` and `power_rail_disable()` correctly drive GPIO 23 and maintain internal tracking flags.
* `test_peripheral_led`: Verifies onboard indicator LED state (GPIO 13) through `led_set()` and `led_toggle()`.
* `test_peripheral_neopixel`: Verifies external NeoPixel initialization on GPIO 21, auto-enablement of the 5V power rail, buffer filling, per-pixel RGB writes, and brightness setting.
* `test_peripheral_servo`: Verifies servo initialization on GPIO 20, auto-enablement of the 5V rail, angle writes, microsecond writes, and detach logic.
* `test_peripheral_buttons`: Verifies active-low digital reading with internal pull-up on the external `Btn` terminal (GPIO 19) and the onboard `BOOT` switch (GPIO 7).
* `test_peripheral_battery`: Verifies 12-bit ADC reads on GPIO 29 / A3, confirming that $(raw / 4095) \times 3.3 \times 2$ accurately computes battery voltage and that battery percentage maps linearly between 3.2V and 4.2V.
* `test_peripheral_motion`: Verifies LIS3DH I2C initialization, 3-axis acceleration extraction ($X, Y, Z$), and single/double tap detection bits.
* `test_peripheral_dynamic_pins`: Verifies runtime configuration of arbitrary GPIO pins (modes, active-high/low output, PWM fading, and pull-up button inputs).
* `test_peripheral_teardown`: Tests the fail-safe cleanup sequence, verifying that user pins return to safe High-Z inputs, servos detach, NeoPixels clear, and the 5V rail cuts power.

#### C. Lua Engine & Script API Bindings
* `test_lua_lifecycle_and_execution`: Tests VM initialization, script execution from virtual FatFS, and clean VM shutdown.
* `test_lua_syntax_error`: Confirms that syntax errors in user scripts are caught gracefully, populate error flags/messages, and do not crash the runtime.
* `test_lua_runtime_error`: Confirms that runtime Lua exceptions (e.g. nil indexing) are safely trapped without crashing the supervisor.
* `test_lua_audio_bindings`: Verifies that Lua API calls (`prop.audio.play`, `prop.audio.set_volume`/`volume`, `prop.audio.tone`, `prop.audio.pause`, `prop.audio.resume`, `prop.audio.stop`) correctly construct and dispatch IPC messages to Core 1.
* `test_lua_audio_is_playing`: Verifies that `prop.audio.is_playing()` accurately reflects the Core 1 audio playback state.
* `test_lua_power_and_peripherals`: Tests Lua script interactions with `prop.power`, `prop.servo`, and `prop.neopixel`.
* `test_lua_status_pixel`: Verifies onboard status NeoPixel control through `prop.status_pixel.set()`, `.rgb()`, `.set_brightness()`, `.off()`, and the `prop.pixel` shorthand aliases.

---

### On-Target Embedded Tests ([`test/test_embedded/test_main.cpp`](test/test_embedded/test_main.cpp))

When executed on physical hardware, these tests verify target-specific operational parameters:

* `test_embedded_system_clock`: Validates that the RP2040 system clock is running at the configured **200 MHz overclock** (`F_CPU == 200000000UL`).
* `test_embedded_ipc_command_queue`: Validates inter-core FIFO memory transfers using the hardware memory bus.
* `test_embedded_power_rail`: Validates electrical switching of the master 5V boost converter on GPIO 23.
* `test_embedded_led_control`: Validates physical control of the red LED on GPIO 13.
* `test_embedded_lua_lifecycle`: Validates Lua 5.4 memory allocation, garbage collector operation, and execution on the RP2040 hardware.

---

## 4. How to Run Tests

### Running Host-Native Unit Tests (Offline)

To run the complete native test suite on your local development machine:

```bash
pio test -e native
```

#### Verbosity Options
If a test fails or you need to inspect debug logs:

```bash
# Level 1: Standard output
pio test -e native -v

# Level 2: Detailed test execution
pio test -e native -vv

# Level 3: Full compiler and linker diagnostic trace
pio test -e native -vvv
```

#### Filtering Tests
To run only a specific test suite folder:

```bash
pio test -e native -f test_native
```

---

### Running On-Target Embedded Tests (Hardware Required)

To execute the test suite on a connected Adafruit RP2040 Prop-Maker Feather:

1. Connect the board to your computer via a data-capable USB-C cable.
2. Run:

```bash
pio test -e feather_rp2040
```

PlatformIO will compile the test binary with Unity, upload it to the board, monitor USB CDC serial at 115200 baud, and display the test results.

#### Verifying Embedded Compilation Without Hardware
To confirm that the embedded test suite compiles cleanly for the RP2040 target without uploading or connecting physical hardware:

```bash
pio test -e feather_rp2040 --without-uploading --without-testing
```

---

### Running Tests in Docker

If you prefer building and testing in a containerized environment:

```bash
# Build the Docker image
docker build -t ezpropkit-builder .

# Run native unit tests inside the container
docker run --rm -v $(pwd):/workspace ezpropkit-builder pio test -e native
```

---

## 5. Guidelines for Contributors

### Writing New Unit Tests

1. **Keep Native Tests Deterministic & Fast**: Host-native tests must run completely offline without relying on real hardware, network access, or long sleeps.
2. **Use Test Isolation**: Always reset mock state in `setUp()` and clean up in `tearDown()`:
   ```cpp
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
   ```
3. **Mocking New Peripherals**: If adding a new sensor or hardware feature:
   * Define public C APIs in `include/peripheral_mgr.h`.
   * Add necessary mock class or function declarations to `test/test_native/mocks/`.
   * Implement mock inspection hooks in `test/test_native/mock_hardware.cpp` (e.g. `mock_set_sensor_value()`).
   * Add a test case in `test/test_native/test_main.cpp` verifying both the C implementation and its corresponding Lua binding in `lua_engine.cpp`.
4. **Preserve Entrypoint Protection**: Never declare global `main()`, `setup()`, or `loop()` functions in library source files (`src/`). In [`src/main.cpp`](src/main.cpp), keep all application entrypoints enclosed in `#ifndef UNIT_TEST` guards.
5. **Verify Both Environments**: Before opening a pull request, always ensure both native unit tests and target firmware builds pass:
   ```bash
   pio test -e native
   pio run -e feather_rp2040
   ```
