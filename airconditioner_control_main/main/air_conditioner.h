#pragma once

#include "ac_types.h"

struct AirConditioner {
  const AcRemote *remote = nullptr;
  AcAction action = AcAction::State;
  AcState state;
};

enum class AcValidationError : uint8_t {
  Ok,
  MissingRemote,
  BadTemp,
  UnsupportedEco,
  UnsupportedSwingV,
  UnsupportedSwingH,
  MissingDriver,
};

void acBeginRemoteDrivers();
AcValidationError acValidate(const AirConditioner &ac);
const char *acValidationCode(AcValidationError error);
const char *acValidationMessage(AcValidationError error);
bool acSend(const AirConditioner &ac);
