#pragma once

#include <Arduino.h>

// System Clock Configuration
#define SYS_CLOCK_HZ            200000000UL // 200 MHz

// Write-Debounce Timer for USB MSC Auto-Reload
#define MSC_WRITE_DEBOUNCE_MS   2000

// Drive Volume Name
#define USB_DRIVE_LABEL         "EZPROPKIT"

// Pin Definitions for Adafruit RP2040 Prop-Maker Feather (PID 5768)
#define PIN_ONBOARD_NEOPIXEL    4   // Built-in WS2812B status RGB LED
#define PIN_ONBOARD_LED         13  // Built-in Red indicator LED
#define PIN_BOOT_BUTTON         7   // Built-in BOOT/User button (active low)

// I2S Audio Pins (MAX98357A Class-D 3W Amplifier)
#define PIN_I2S_DATA            16  // DIN
#define PIN_I2S_BCLK            17  // Bit Clock
#define PIN_I2S_LRCLK           18  // Word Select / LRCLK

// Terminal Block and Header Pins
#define PIN_EXTERNAL_BUTTON     19  // 'Btn' screw terminal (active low with pull-up)
#define PIN_EXTERNAL_SERVO      20  // 'Sig' 3-pin servo header (PWM)
#define PIN_EXTERNAL_NEOPIXELS  21  // 'Neo' screw terminal (5V level shifted)
#define PIN_ACCEL_INTERRUPT     22  // LIS3DH INT1 interrupt line
#define PIN_EXTERNAL_POWER      23  // Master Power Enable (5V booster, I2S amp, servo, LEDs)

// I2C Pins (LIS3DH Accelerometer & STEMMA QT connector)
#define PIN_I2C_SDA             2
#define PIN_I2C_SCL             3

// Analog Inputs
#define PIN_BATTERY_ADC         29  // Feather A3 / ADC3 (1/2 LiPo voltage divider)

// Stabilization delay after enabling 5V power rail (prevents inrush brownout)
#define POWER_ENABLE_DELAY_MS   5
