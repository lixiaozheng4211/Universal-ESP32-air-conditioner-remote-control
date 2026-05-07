#include "drivers/ir_test.h"

#include "ac_config.h"

#include <Arduino.h>
#include <IRsend.h>
#include <ir_NEC.h>

namespace {

constexpr uint16_t kMaxCarrierMarkUsec = 16000;

IRsend gTestIr(kAcIrLedGpio);

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

void sendNecFrame(const IrTestRequest &request) {
  gTestIr.sendGeneric(kNecHdrMark, kNecHdrSpace, kNecBitMark, kNecOneSpace,
                      kNecBitMark, kNecZeroSpace, kNecBitMark, kNecMinGap,
                      kNecMinCommandLength, request.data, kNECBits,
                      request.freqHz, true, request.count - 1,
                      request.dutyPercent);
}

} // namespace

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
