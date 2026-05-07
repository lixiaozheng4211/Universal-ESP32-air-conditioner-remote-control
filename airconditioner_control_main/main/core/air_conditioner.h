#pragma once

#include "ac_types.h"

// Runtime command object created from one AC serial command. The selected
// remote carries the backend function table; action decides full vs single send.
struct AirConditioner {
  const AcRemote *remote = nullptr;
  AcAction action = AcAction::State;
  AcState state;
};

// Validation errors are kept separate from serial parsing so the same checks
// protect Qt, Android, and manual terminal commands.
enum class AcValidationError : uint8_t {
  Ok,
  MissingRemote,
  BadTemp,
  UnsupportedFan,
  UnsupportedSwingV,
  UnsupportedSwingH,
  MissingDriver,
};

// Initialize all distinct remote driver classes declared in the catalog.
void acBeginRemoteDrivers();

// Check a command against the selected remote capabilities before sending.
AcValidationError acValidate(const AirConditioner &ac);
const char *acValidationCode(AcValidationError error);
const char *acValidationMessage(AcValidationError error);

// Dispatch through the selected backend. Special remotes may implement
// sendAction to avoid emitting several IR frames for one UI control change.
bool acSend(const AirConditioner &ac);
