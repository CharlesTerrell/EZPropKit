# Open Source Third-Party Licenses & Compliance Review

This document provides upstream repository links, license types, and copyright notices for all third-party libraries, cores, drivers, and bundled assets referenced or used by **`ezpropkit`**.

---

## 1. Third-Party License Matrix

| Component | Upstream Author / Maintainer | Repository / Source | License | SPDX Identifier |
| :--- | :--- | :--- | :--- | :--- |
| **BackgroundAudio (WAV & I2S Engine)** | Earle F. Philhower, III | [earlephilhower/BackgroundAudio](https://github.com/earlephilhower/BackgroundAudio) | GNU General Public License v3.0 | `GPL-3.0-only` |
| **libmad MP3 Decoder** | Underbit Technologies / Robert Leslie (Ported by Earle Philhower) | Bundled in `BackgroundAudio/src/libmad` | GNU General Public License v2.0 or later | `GPL-2.0-or-later` |
| **Lua 5.4 Core Engine** | Roberto Ierusalimschy, Waldemar Celes, Luiz Henrique de Figueiredo | [Lua.org](https://www.lua.org) | MIT License | `MIT` |
| **arduino-lua (PlatformIO Package)** | Nick Owens (`mischief`) | [mischief/arduino-lua](https://github.com/mischief/arduino-lua) | MIT License | `MIT` |
| **arduino-pico Core** | Earle F. Philhower, III & Contributors | [earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico) | GNU Lesser General Public License v2.1 | `LGPL-2.1-only` |
| **Raspberry Pi Pico SDK** | Raspberry Pi (Trading) Ltd | [raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk) (via `arduino-pico`) | BSD 3-Clause License | `BSD-3-Clause` |
| **platform-raspberrypi** | Maximilian Gerhardt | [maxgerhardt/platform-raspberrypi](https://github.com/maxgerhardt/platform-raspberrypi) | Apache License 2.0 | `Apache-2.0` |
| **Adafruit TinyUSB Library & TinyUSB** | Ha Thach & Adafruit Industries | [adafruit/Adafruit_TinyUSB_Arduino](https://github.com/adafruit/Adafruit_TinyUSB_Arduino) / [hathach/tinyusb](https://github.com/hathach/tinyusb) | MIT License | `MIT` |
| **FatFS** | ChaN | [elm-chan.org](http://elm-chan.org/fsw/ff/00index_e.html) (via `arduino-pico`) | ChaN FatFS License (BSD-style permissive) | `FatFS` |
| **Adafruit NeoPixel** | Phil Burgess & Adafruit Industries | [adafruit/Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) | GNU Lesser General Public License v3.0 | `LGPL-3.0-only` |
| **Adafruit LIS3DH** | Adafruit Industries | [adafruit/Adafruit_LIS3DH](https://github.com/adafruit/Adafruit_LIS3DH) | BSD 3-Clause License | `BSD-3-Clause` |
| **Adafruit BusIO** | Adafruit Industries | [adafruit/Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO) (dependency of LIS3DH) | MIT License | `MIT` |
| **Adafruit Unified Sensor** | Adafruit Industries | [adafruit/Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor) | Apache License 2.0 | `Apache-2.0` |
| **Servo(rp2040) & I2S** | Earle F. Philhower, III | Bundled in `arduino-pico` core | GNU Lesser General Public License v2.1 | `LGPL-2.1-only` |

### Unused / Optional Upstream Components

| Component | Upstream Project | License | Status in ezpropkit |
| :--- | :--- | :--- | :--- |
| **Helix AAC Decoder (`libhelix-aac`)** | RealNetworks | RealNetworks Public Source License 1.0 (`RPSL-1.0`) / RCSL | Bundled in upstream `BackgroundAudio`, but **not compiled / omitted** in `ezpropkit` firmware builds to conserve RP2040 SRAM (~106 KB savings). |
| **eSpeak-NG (`libespeak-ng`)** | Jonathan Duddington / eSpeak-NG team | GNU General Public License v3.0 (`GPL-3.0-only`) | Bundled in upstream `BackgroundAudio`, but **not linked** in `ezpropkit`. |
| **MicroLua** | Remy Blank ([MicroLua/MicroLua](https://github.com/MicroLua/MicroLua)) | MIT License (`MIT`) | Evaluated during early architecture planning; **not used** in the current build (PUC-Rio Lua 5.4 via `mischief/lua` was chosen instead). |

---

## 2. Detailed Component Review & Compliance Notes

### 2.1 BackgroundAudio & libmad
* **Upstream:** [earlephilhower/BackgroundAudio](https://github.com/earlephilhower/BackgroundAudio)
* **Authors:** Earle F. Philhower, III (BackgroundAudio, WAV streaming, RP2040 libmad port); Underbit Technologies / Robert Leslie (libmad MP3 decoder).
* **Licenses:** GNU General Public License v3.0 (`BackgroundAudio`) & GNU General Public License v2.0 or later (`libmad`).
* **Compliance Requirement**: Because `BackgroundAudio` is licensed under GPLv3 and is statically linked into the firmware image:
  - **Any pre-compiled binary releases (`.uf2`, `.bin`, `.elf`) of `ezpropkit` must be distributed under the GNU General Public License v3.0 (GPL-3.0)**.
  - The complete corresponding source code of `ezpropkit` must be made available to anyone receiving the binary, as stipulated by GPLv3 Section 6.

### 2.2 Lua 5.4 Core Engine & arduino-lua
* **Upstream:** [Lua.org](https://www.lua.org/license.html) & [mischief/arduino-lua](https://github.com/mischief/arduino-lua)
* **Authors:** Roberto Ierusalimschy, Waldemar Celes, Luiz Henrique de Figueiredo (PUC-Rio); Nick Owens (`mischief`).
* **License:** MIT License (`MIT`)
* **Copyright:** Copyright &copy; 1994–2024 Lua.org, PUC-Rio. Copyright &copy; Nick Owens.
* **Compliance Requirement**: The standard MIT copyright notice and disclaimer are preserved in source distributions.

### 2.3 arduino-pico (RP2040 Arduino Core)
* **Upstream:** [earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico)
* **Author:** Earle F. Philhower, III & Contributors
* **License:** GNU Lesser General Public License v2.1 (`LGPL-2.1-only`)
* **Copyright:** Copyright &copy; 2021–2025 Earle F. Philhower, III
* **Compliance Requirement**: Source code of the core is freely available. Under GPL compatibility provisions, statically linking an LGPLv2.1 library into a GPLv3 composite work is explicitly permissible.

### 2.4 Raspberry Pi Pico SDK
* **Upstream:** [raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk)
* **Author:** Raspberry Pi (Trading) Ltd
* **License:** BSD 3-Clause License (`BSD-3-Clause`)
* **Copyright:** Copyright &copy; 2020 Raspberry Pi (Trading) Ltd.
* **Compliance Requirement**: BSD 3-Clause copyright notice and disclaimer retained.

### 2.5 platform-raspberrypi
* **Upstream:** [maxgerhardt/platform-raspberrypi](https://github.com/maxgerhardt/platform-raspberrypi)
* **Author:** Maximilian Gerhardt
* **License:** Apache License 2.0 (`Apache-2.0`)
* **Copyright:** Copyright 2021–2025 Maximilian Gerhardt

### 2.6 Adafruit Libraries (NeoPixel, LIS3DH, Sensor, BusIO, TinyUSB)
* **Upstream:** [Adafruit Industries GitHub](https://github.com/adafruit)
* **Authors:** Adafruit Industries & community contributors
* **Licenses:**
  - `Adafruit_NeoPixel`: LGPLv3 (`LGPL-3.0-only`)
  - `Adafruit_LIS3DH`: BSD 3-Clause (`BSD-3-Clause`)
  - `Adafruit_BusIO`: MIT License (`MIT`)
  - `Adafruit_Sensor`: Apache License 2.0 (`Apache-2.0`)
  - `Adafruit_TinyUSB_Arduino` & `tinyusb`: MIT License (`MIT`)

---

## 3. Project Licensing & Repository Breakdown

The `ezpropkit` project organizes licensing across its repository as follows:

```
ezpropkit/
├── LICENSE                    # GNU General Public License v3.0 (Firmware binary & runtime C/C++ source)
├── THIRD_PARTY_LICENSES.md    # Upstream open-source license review (this document)
├── AI_POLICY.md               # Policy regarding human-in-the-loop AI assistance
├── examples/
│   ├── LICENSE                # MIT No Attribution (MIT-0) for all example Lua scripts
│   ├── sound_credits.md       # Attribution for CC0 audio files
│   ├── blaster/               # Sample scripts (MIT-0) & CC0 sound files
│   └── readme-example/        # Sample scripts (MIT-0) & CC0 sound files
└── tools/
    ├── LICENSE                # MIT No Attribution (MIT-0) for developer utility scripts
    └── ffm2mp3                # MP3 conversion utility script (MIT-0)
```

### 3.1 Composite Firmware Binary (`.uf2`)
The compiled binary links `BackgroundAudio` (GPLv3), `libmad` (GPLv2+), `arduino-pico` (LGPLv2.1), `Adafruit_NeoPixel` (LGPLv3), and permissive libraries (Lua, TinyUSB, FatFS, Pico SDK). The composite work is governed by the **GNU General Public License v3.0 (GPL-3.0)**.

### 3.2 User Lua Scripts & Prop Maker Code
User scripts executed by the Lua interpreter (placed on the `EZPROPKIT` USB volume) are independent interpretive works. Prop makers may license, distribute, sell, or keep proprietary their custom Lua scripts (`code.lua`) under whatever terms they choose.

### 3.3 Example Code (`examples/`) & Tools (`tools/`)
All sample Lua scripts and utility scripts are licensed under **MIT No Attribution (`MIT-0`)** (see [`examples/LICENSE`](file:///home/charlie/Code/ezpropkit-project/ezpropkit/examples/LICENSE) and [`tools/LICENSE`](file:///home/charlie/Code/ezpropkit-project/ezpropkit/tools/LICENSE)). Users may copy, modify, and integrate example scripts into their own projects without attribution or copyleft obligations.

### 3.4 Audio Assets (`examples/`)
Audio samples bundled in `examples/` are public domain works dedicated under **Creative Commons Zero 1.0 Universal (`CC0-1.0`)**. See [`examples/sound_credits.md`](file:///home/charlie/Code/ezpropkit-project/ezpropkit/examples/sound_credits.md) for individual Freesound credits and source links:
* `hum.mp3` by chungus43A (CC0)
* `blaster.wav` by MikeE63 (CC0)
* `powerup.mp3` by GammaGool (CC0)

---

## 4. Verification Commands

To verify upstream repository branches and commit hashes:

```bash
# Verify BackgroundAudio master branch
git ls-remote https://github.com/earlephilhower/BackgroundAudio.git HEAD

# Verify arduino-lua PlatformIO package
git ls-remote https://github.com/mischief/arduino-lua.git HEAD

# Verify arduino-pico core
git ls-remote https://github.com/earlephilhower/arduino-pico.git HEAD

# Verify platform-raspberrypi repository
git ls-remote https://github.com/maxgerhardt/platform-raspberrypi.git HEAD
```
