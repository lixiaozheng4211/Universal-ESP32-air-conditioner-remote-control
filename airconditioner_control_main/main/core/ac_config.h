#pragma once

#include <stdint.h>

// Central firmware constants shared by the serial protocol and IR drivers.
// Keep these values stable unless the hardware wiring or baud rate changes.
constexpr uint8_t kAcIrLedGpio = 4;
constexpr uint32_t kAcSerialBaud = 115200;

// IRremoteESP8266 uses protocol-specific model numbers. This sentinel means
// the selected protocol does not need a concrete remote model.
constexpr int16_t kAcNoModel = -1;
