#pragma once

#include <stdint.h>

enum class IrTestMode {
  Nec,
  Carrier,
};

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
