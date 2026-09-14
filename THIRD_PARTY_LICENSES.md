# Open Source Third-Party Licenses & Compliance Review

This document provides complete upstream repository links, license types, and copyright notices for all third-party libraries, cores, and drivers referenced or used by **`ezpropkit`**.

---

## License Matrix Summary

| Component | Upstream Author / Project | Repository URL | License | SPDX Identifier |
| :--- | :--- | :--- | :--- | :--- |
| **BackgroundAudio** | Earle F. Philhower, III | https://github.com/earlephilhower/BackgroundAudio | GNU General Public License v3.0 | `GPL-3.0-only` |
| **Helix MP3 & AAC Decoders** | RealNetworks / Earle Philhower | Bundled in `BackgroundAudio` | RealNetworks Public Source / GPL | `RPSL-1.0` / `GPL-3.0` |
| **arduino-pico** | Earle F. Philhower, III | https://github.com/earlephilhower/arduino-pico | GNU Lesser General Public License v2.1 | `LGPL-2.1-only` |
| **platform-raspberrypi** | Maximilian Gerhardt | https://github.com/maxgerhardt/platform-raspberrypi | Apache License 2.0 | `Apache-2.0` |
| **MicroLua** | Remy Blank | https://github.com/MicroLua/MicroLua | MIT License | `MIT` |
| **Lua 5.4** | PUC-Rio (Lua.org) | https://www.lua.org / https://github.com/lua/lua | MIT License | `MIT` |
| **TinyUSB** | Ha Thach (Adafruit) | https://github.com/hathach/tinyusb | MIT License | `MIT` |
| **FatFS** | ChaN | http://elm-chan.org/fsw/ff/00index_e.html | ChaN FatFS License (BSD-like) | `FatFS` |
| **Adafruit NeoPixel** | Adafruit Industries | https://github.com/adafruit/Adafruit_NeoPixel | GNU Lesser General Public License v3.0 | `LGPL-3.0` |
| **Adafruit LIS3DH** | Adafruit Industries | https://github.com/adafruit/Adafruit_LIS3DH | BSD 3-Clause License | `BSD-3-Clause` |
| **Adafruit Unified Sensor** | Adafruit Industries | https://github.com/adafruit/Adafruit_Sensor | Apache License 2.0 | `Apache-2.0` |

---

## Detailed Component Review & Compliance Notes

### 1. BackgroundAudio
* **Upstream:** [earlephilhower/BackgroundAudio](https://github.com/earlephilhower/BackgroundAudio)
* **Author:** Earle F. Philhower, III
* **License:** GNU General Public License v3.0 (GPL-3.0)
* **Copyright:** Copyright (c) 2024–2025 Earle F. Philhower, III
* **Compliance Requirement**: Because `BackgroundAudio` is licensed under GPLv3 and is linked directly into the firmware image, **any pre-compiled binary releases (`.uf2`, `.bin`, `.hex`) of `ezpropkit` must be distributed under the GNU GPL v3 license**. The source code of `ezpropkit` must be made available to recipients of the binary.

### 2. MicroLua
* **Upstream:** [MicroLua/MicroLua](https://github.com/MicroLua/MicroLua)
* **Author:** Remy Blank (`remy@c-space.org`)
* **License:** MIT License
* **Copyright:** Copyright (c) 2023 Remy Blank
* **Compliance Requirement**: Include MIT copyright and permission notice in distributions.

### 3. arduino-pico (Raspberry Pi Pico Arduino Core)
* **Upstream:** [earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico)
* **Author:** Earle F. Philhower, III & Contributors
* **License:** GNU Lesser General Public License v2.1 (LGPL-2.1)
* **Copyright:** Copyright (c) 2021–2025 Earle F. Philhower, III
* **Compliance Requirement**: Source code of the core is open and available. Static linking into GPLv3 composite binaries is permissible under GPL compatibility provisions.

### 4. platform-raspberrypi (PlatformIO Integration)
* **Upstream:** [maxgerhardt/platform-raspberrypi](https://github.com/maxgerhardt/platform-raspberrypi)
* **Author:** Maximilian Gerhardt
* **License:** Apache License 2.0
* **Copyright:** Copyright 2021–2025 Maximilian Gerhardt

### 5. Lua 5.4 Core Engine
* **Upstream:** [Lua.org](https://www.lua.org/license.html)
* **Authors:** Roberto Ierusalimschy, Waldemar Celes, Luiz Henrique de Figueiredo
* **License:** MIT License
* **Copyright:** Copyright (C) 1994–2026 Lua.org, PUC-Rio.
* **Compliance Requirement**: Standard MIT copyright notice must be retained.

---

## Instructions for Re-verifying Upstream Sources

To inspect or update upstream dependencies:
```bash
# Verify BackgroundAudio master branch
git ls-remote https://github.com/earlephilhower/BackgroundAudio.git HEAD

# Verify MicroLua master branch
git ls-remote https://github.com/MicroLua/MicroLua.git HEAD

# Verify platform-raspberrypi repository
git ls-remote https://github.com/maxgerhardt/platform-raspberrypi.git HEAD
```
