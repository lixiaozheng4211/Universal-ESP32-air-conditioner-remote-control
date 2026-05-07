#include "ac_types.h"

const char *acModeToString(stdAc::opmode_t mode) {
  switch (mode) {
  case stdAc::opmode_t::kAuto:
    return "auto";
  case stdAc::opmode_t::kCool:
    return "cool";
  case stdAc::opmode_t::kHeat:
    return "heat";
  case stdAc::opmode_t::kDry:
    return "dry";
  case stdAc::opmode_t::kFan:
    return "fan";
  default:
    return "unknown";
  }
}

const char *acActionToString(AcAction action) {
  switch (action) {
  case AcAction::State:
    return "state";
  case AcAction::Power:
    return "power";
  case AcAction::Temp:
    return "temp";
  case AcAction::Mode:
    return "mode";
  case AcAction::SwingV:
    return "swingv";
  default:
    return "unknown";
  }
}
