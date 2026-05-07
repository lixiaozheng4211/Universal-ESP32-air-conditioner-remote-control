#pragma once

#include <stdint.h>

// 底层红外诊断命令。Nec 模式适合让普通红外接收头看到可解码帧；
// Carrier 模式适合用接收器或示波器检查原始 38K/40K 载波。
enum class IrTestMode {
  Nec,
  Carrier,
};

// 默认值适合从 Qt 或串口终端快速做硬件测试。
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
