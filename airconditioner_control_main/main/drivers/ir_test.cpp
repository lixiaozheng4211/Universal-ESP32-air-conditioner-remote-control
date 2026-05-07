#include "drivers/ir_test.h"

#include "ac_config.h"

#include <Arduino.h>
#include <IRsend.h>
#include <ir_NEC.h>

namespace {

constexpr uint16_t kMaxCarrierMarkUsec = 16000;

IRsend gTestIr(kAcIrLedGpio);

// 开启载波前先校验参数，避免错误串口输入让红外 LED 长时间工作。
bool isValidRequest(const IrTestRequest &request) {
  if (request.freqHz < 30000 || request.freqHz > 60000) {
    return false;
  }
  if (request.count == 0 || request.count > 20) {
    return false;
  }
  if (request.carrierMs == 0 || request.carrierMs > 2000) {
    return false;
  }
  return request.dutyPercent >= 10 && request.dutyPercent <= 80;
}

// IRremoteESP8266 的 mark() 接收 uint16_t 微秒值。
// 长时间载波会拆成安全的小段，同时保持看起来连续的载波输出。
void sendCarrier(uint32_t freqHz, uint16_t carrierMs, uint8_t dutyPercent) {
  gTestIr.enableIROut(freqHz, dutyPercent);

  uint32_t remainingUsec = static_cast<uint32_t>(carrierMs) * 1000;
  while (remainingUsec > 0) {
    const uint16_t chunk =
        remainingUsec > kMaxCarrierMarkUsec
            ? kMaxCarrierMarkUsec
            : static_cast<uint16_t>(remainingUsec);
    gTestIr.mark(chunk);
    remainingUsec -= chunk;
  }
  gTestIr.space(0);
}

// NEC 是常见且容易识别的测试协议。
// 在匹配空调协议前，可先确认接收器能解出一帧可识别数据。
void sendNecFrame(const IrTestRequest &request) {
  gTestIr.sendGeneric(kNecHdrMark, kNecHdrSpace, kNecBitMark, kNecOneSpace,
                      kNecBitMark, kNecZeroSpace, kNecBitMark, kNecMinGap,
                      kNecMinCommandLength, request.data, kNECBits,
                      request.freqHz, true, request.count - 1,
                      request.dutyPercent);
}

} // 命名空间

void irTestBegin() { gTestIr.begin(); }

bool irTestSend(const IrTestRequest &request) {
  if (!isValidRequest(request)) {
    return false;
  }

  irTestBegin();
  if (request.mode == IrTestMode::Carrier) {
    for (uint16_t i = 0; i < request.count; ++i) {
      sendCarrier(request.freqHz, request.carrierMs, request.dutyPercent);
      delay(80);
    }
    return true;
  }

  sendNecFrame(request);
  return true;
}

const char *irTestModeToString(IrTestMode mode) {
  switch (mode) {
  case IrTestMode::Carrier:
    return "carrier";
  case IrTestMode::Nec:
  default:
    return "nec";
  }
}
