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
