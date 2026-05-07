#pragma once

#include <stdint.h>

// Low-level IR diagnostic command. Use Nec for receiver-visible frames and
// Carrier for checking raw 38K/40K output with an IR receiver or oscilloscope.
enum class IrTestMode {
  Nec,
  Carrier,
};

// Defaults are safe for quick hardware tests from Qt or a serial terminal.
struct IrTestRequest {
  uint32_t freqHz = 38000;
  uint16_t count = 3;
  uint16_t carrierMs = 300;
  uint8_t dutyPercent = 33;
  uint64_t data = 0x00FF00FF;
  IrTestMode mode = IrTestMode::Nec;
};

void irTestBegin();
bool irTestSend(const IrTestRequest &request);
const char *irTestModeToString(IrTestMode mode);
