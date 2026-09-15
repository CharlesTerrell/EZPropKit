# EZPropKit: Lua & C Firmware for Adafruit RP2040 Prop-Maker Feather

**EZPropKit** is an open-source, dual-core firmware engine designed for rapid prototyping of animatronics and costume props using the [**Adafruit RP2040 Prop-Maker Feather**](https://learn.adafruit.com/adafruit-rp2040-prop-maker-feather). These boards have several useful features included for a reasonably low price (US$20 in 2026). Support for other boards with similar feature sets can be added as needed.

It delivers a CircuitPython-like workflow using **Lua 5.4**:
1. Connect the board via USB: it mounts as a standard flash drive named `EZPROPKIT`
2. Drag and drop audio files (`.wav`, `.mp3`) and Lua scripts (`code.lua` or `main.lua`)
3. Edit scripts in place. When you save, the board automatically detects write completion and hot-reloads your code.
4. If a script encounters a runtime error, the board remains mounted over USB, pulses the onboard NeoPixel red/orange, and prints a stack trace to the USB serial console (`115200 baud`).

Under the hood, **Core 0** runs the USB stack and interpreted Lua VM, while **Core 1** runs an optimized C engine handling I2S audio decoding/DMA, PIO NeoPixel generation, and servo timing without jitter.

Lua was chosen for the user code interpreter because:
- Lua is relatively easy to integrate with C/C++ libraries
- Its syntax is easy to learn and read
- Python has already been done

This project was also intended as a learning exercise in using AI agent workflows. The initial code was planned and implemented by Gemini under the close supervision of the project's creator, a long-time programmer in C-like languages. This workflow allowed the project to go from idea to working prototype in less than a week. There were several informative technical conversations along the way about why certain changes were necessary and which features could fit in the RP2040's limited memory. The human then examined the core source code, fleshed out the Lua examples, and rewrote parts of the documentation. `AI_POLICY.md` has info about use of AI tools for future contributions.

---

## Hardware Specifications & Pinout

Target Board: **Adafruit RP2040 Prop-Maker Feather** (SKU 5768)  
MCU: **RP2040** (Dual ARM Cortex-M0+ @ 200 MHz, 264 KB SRAM, 8 MB QSPI Flash)  
Compiler Flags: `-O3 -DUSE_TINYUSB`

| Pin / Label | Peripheral Function | Hardware Connection / Notes |
| :--- | :--- | :--- |
| **GPIO 4** | Onboard NeoPixel | Status / safe-mode RGB LED |
| **GPIO 13** | Onboard Red LED | Standard activity LED |
| **GPIO 7** | Onboard Button / BOOT | User push button (active low). Also used during reset/power-up to enter ROM bootloader mode (`RPI-RP2`). |
| **GPIO 16** | I2S DIN (Data) | MAX98357A Class-D 3W Audio Amplifier |
| **GPIO 17** | I2S BCLK (Bit Clock) | Audio bit clock |
| **GPIO 18** | I2S LRCLK (Word Select)| Audio left/right channel clock |
| **GPIO 19** | `Btn` Screw Terminal | External trigger/button input (pull-up) |
| **GPIO 20** | `Sig` Servo Header | Servo PWM output (50 Hz signal) |
| **GPIO 21** | `Neo` Screw Terminal | External NeoPixel data (buffered via 5V level shifter) |
| **GPIO 22** | Accelerometer IRQ | LIS3DH triple-axis motion sensor interrupt |
| **GPIO 23** | `EXTERNAL_POWER` | Master power switch for 5V boost, I2S amp, servo rail, NeoPixels |
| **GPIO 2** | I2C SDA | LIS3DH accelerometer & STEMMA QT / Qwiic |
| **GPIO 3** | I2C SCL | LIS3DH accelerometer & STEMMA QT / Qwiic |
| **GPIO 29** (`A3`) | Battery Sense ADC | 1/2 voltage divider connected to LiPo battery JST |

> **Note on Power Rail (GPIO 23)**: To conserve battery and prevent sleep drain, the 5V booster, I2S amp, external NeoPixel terminal, and servo power rail remain unpowered until requested by your Lua script or the peripheral manager.

---

## User Workflow (Prop Makers)

Prop makers do **not** need to compile code or install toolchains. To install:

1. Hold the board's `BOOT` button while plugging it into your computer with a USB cable. Release the button after plugging it in.
2. The drive `RPI-RP2` appears on your computer. Drag the pre-compiled `ezpropkit-feather-rp2040.uf2` (available from the GitHub Releases page) onto the root of `RPI-RP2`. If it doesn't work, check to be sure the cable can handle data and not just charging. Adafruit's [CircuitPython Quickstart](https://learn.adafruit.com/adafruit-rp2040-prop-maker-feather/circuitpython) page has more info about flashing firmware images.
3. After the firmware image finishes copying, `RPI-RP2` will disappear and be replaced with a drive named `EZPROPKIT`.
4. Copy (drag-drop or 'cp') your project's sound files (`blaster.wav`, `powerup.mp3`) onto the root of `EZPROPKIT`.
5. Create or edit `code.lua` on `EZPROPKIT`. For example, this can be found (with the sound files listed above) in `examples/readme-example`:

```lua
-- Simple interactive prop example
prop.led.on()
prop.power.enable() -- Powers 5V boost rail, amp, and external LEDs

-- Initialize 16 external NeoPixels on the terminal block
-- (16 LEDs in my neopixel ring; change to match how many are in yours)
local strip = prop.neopixel.init(16)
strip:fill(0, 100, 255)
strip:show()

-- Play sound file in background
prop.audio.play("powerup.mp3")

-- Listen to the screw-terminal button
while true do
    if prop.button.pressed() then
        prop.audio.play("blaster.wav")
        strip:fill(255, 50, 0)
        strip:show()
        prop.time.sleep_ms(150)
        strip:fill(0, 100, 255)
        strip:show()
    end
    prop.time.sleep_ms(20)
end
```

6. When you save `code.lua`, the board automatically reloads within ~2 seconds.

---

## Lua `prop.*` API Summary

All hardware peripherals are lazily initialized on first access:

* **`prop.audio`**:
  * `prop.audio.play(filename, [loop=false])`: Streams `.wav` (16-bit 44.1 kHz PCM) or `.mp3` asynchronously from flash via I2S.
  * `prop.audio.tone(freq_hz, duration_ms)`: Plays a pure tone via I2S (useful for chirps, power-up beeps, and audio feedback).
  * `prop.audio.pause()` / `prop.audio.resume()`: Toggles playback.
  * `prop.audio.stop()`: Stops audio and releases I2S DMA buffers.
  * `prop.audio.set_volume(0..100)`: Adjusts software volume.
  * `prop.audio.is_playing()`: Returns boolean indicating if audio is streaming.
* **`prop.neopixel`**:
  * `strip = prop.neopixel.init(count, [pin=21])`: Allocates PIO state machine for WS2812B.
  * `strip:set(index, r, g, b)`: Sets pixel RGB (0..255).
  * `strip:fill(r, g, b)`: Fills all pixels.
  * `strip:show()`: Transmits buffer via PIO DMA.
  * `strip:clear()`: Clears all pixels and sends update.
* **`prop.servo`**:
  * `s = prop.servo.init([pin=20], [min_us=500], [max_us=2500])`: Configures PWM slice.
  * `s:angle(degrees)`: Positions servo between 0° and 180°.
  * `s:pulse(microseconds)`: Sets raw pulse width.
  * `s:detach()`: Releases PWM hardware to save power.
* **`prop.power`**:
  * `prop.power.enable()`: Sets GPIO 23 HIGH (turns on 5V rail, I2S amp, servo power).
  * `prop.power.disable()`: Sets GPIO 23 LOW (cuts quiescent draw).
* **`prop.motion`**:
  * `prop.motion.read_accel()`: Returns `x, y, z` in m/s² from onboard LIS3DH.
  * `prop.motion.is_tapped()` / `prop.motion.is_shaken()`: Quick gesture flags.
* **`prop.battery`**:
  * `prop.battery.voltage()`: Returns LiPo voltage (e.g. `3.85`).
  * `prop.battery.percent()`: Estimated charge percentage (0..100).
* **`prop.button`**:
  * `prop.button.pressed()`: Debounced state of external `Btn` terminal (GPIO 19).
  * `prop.button.boot_pressed()`: Debounced state of onboard `BOOT` switch (GPIO 7).
  * `btn = prop.button.new(pin, [pull_mode="up"|"down"|"none"], [active_low=true])`: Creates a dynamic button on any GPIO pin.
  * `btn:pressed()`: Returns debounced boolean state of custom button.
* **`prop.led`**:
  * `prop.led.on()`, `prop.led.off()`, `prop.led.toggle()`: Controls red onboard LED (GPIO 13).
  * `led = prop.led.new(pin, [active_high=true])`: Creates a dynamic LED on any GPIO pin.
  * `led:on()`, `led:off()`, `led:toggle()`: Controls custom LED.
  * `led:pwm(0..255)`: Dims / fades custom LED using hardware PWM.
* **`prop.gpio`**:
  * `prop.gpio.mode(pin, "in_pullup"|"in_pulldown"|"input"|"output")`: Configures pin mode.
  * `prop.gpio.read(pin)` / `prop.gpio.write(pin, bool)`: Digital I/O.
  * `prop.gpio.pwm(pin, 0..255)`: PWM output on any pin.
  * `prop.gpio.adc(pin)`: 12-bit analog reading on ADC pins (26..28).
* **`prop.time`**:
  * `prop.time.sleep_ms(ms)`: Non-blocking cooperative sleep (yields to coroutines & USB background poll).
  * `prop.time.ticks_ms()`: Milliseconds since boot.

---

## Developer Guide: Building Firmware

### Path A: Native PlatformIO (Recommended for local flashing & debugging)

Requires Python 3 and PlatformIO Core:

```bash
pip install -U platformio
pio run -e feather_rp2040
```

To flash via USB bootloader:
```bash
pio run -e feather_rp2040 -t upload
```

To open the serial log monitor:
```bash
pio device monitor -b 115200
```

### Path B: Headless Docker Build (Ideal for CI/CD & consistent environments)

Build the firmware inside a container without installing local toolchains (requires Docker with the `docker-buildx` plugin on Linux hosts, e.g., `sudo apt install docker-buildx`):

```bash
docker build -t ezpropkit-builder .
docker run --rm -v $(pwd):/workspace ezpropkit-builder pio run -e feather_rp2040
```

The compiled UF2 binary will be located at:
`.pio/build/feather_rp2040/firmware.uf2`

#### Docker, Antigravity CLI & Agent Auth

If running development agents or the Antigravity CLI within Docker:
* Mount your project directory: `-v $(pwd):/workspace`
* Mount your auth tokens read-only: `-v ~/.gemini:/root/.gemini:ro`
* *Note on USB serial on macOS/Windows*: Docker Desktop runs inside a VM and cannot natively pass host USB serial ports without third-party USB-over-IP daemons (`usbipd`). Use native PlatformIO on the host for real-time serial streaming, and use Docker for reproducible compilation and CI.

---

## License & Attribution

* **Lua 5.4** (PUC-Rio core via mischief/arduino-lua): MIT License ([Lua.org](https://www.lua.org) / [mischief/arduino-lua](https://github.com/mischief/arduino-lua))
* **BackgroundAudio** (Audio decoders & I2S engine): GNU GPL v3 ([earlephilhower/BackgroundAudio](https://github.com/earlephilhower/BackgroundAudio))
* **libmad** (MPEG audio decoder): GNU GPL v2+ (Underbit Technologies / Robert Leslie)
* **arduino-pico** (RP2040 Arduino core): LGPL-2.1 ([earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico))
* **platform-raspberrypi** (PlatformIO integration): Apache-2.0 ([maxgerhardt/platform-raspberrypi](https://github.com/maxgerhardt/platform-raspberrypi))
* **TinyUSB** (USB MSC & CDC device stack): MIT License
* **FatFS** (FAT12/16/32 filesystem library): ChaN FatFS License

Due to the inclusion of `BackgroundAudio` (GPL v3) and `arduino-pico` (statically linked LGPL 2.1), the compiled binary of **EZPropKit** is distributed under the **GNU General Public License v3.0 (GPL-3.0)**. `LICENSE` has the license text. For simplicity the project's source code files are also under GPL v3 with these exceptions:

Source code files in the `examples` folder are under the **MIT-0 license**. This allows you to use the examples in your own prop-making projects (or for any other purpose). You are not required to publicly share any changes or additions you make to the examples. You are also not forbidden from sharing if you choose to do so. Please read `examples/LICENSE` for further details, including disclaimers of warranty or liability.

This is mostly of interest if you want to give or sell your prop and its software to other people. In that situation, the compiled binary (firmware) remains under GPL v3, and any changes you make to it must be shared as described in the license. Your specific application code (run by the Lua interpreter) can be shared or kept private under whatever terms you prefer.

Example sound files have their own license terms, usually **Creative Commons Zero**. Details are in `examples/sound_credits.md`. Big thanks to all who share their sound effects under such generous terms.

Utility scripts in the `tools` folder are also under the **MIT-0 license**. Feel free to use or learn from them. Much of it came from online sample code anyway.

