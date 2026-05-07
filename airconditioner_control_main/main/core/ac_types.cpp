#include "ac_types.h"

// 把固件内部枚举转换回协议字符串，用于 OK 响应。
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

// 保持响应文本和 action= 可接受的取值一致。
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
  case AcAction::Fan:
    return "fan";
  case AcAction::SwingV:
    return "swingv";
  case AcAction::SwingH:
    return "swingh";
  default:
    return "unknown";
  }
}
