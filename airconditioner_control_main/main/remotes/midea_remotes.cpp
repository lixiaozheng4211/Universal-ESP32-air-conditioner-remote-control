#include "remotes/midea_remotes.h"

#include "drivers/irac_backend.h"
#include "ir_Midea2.h"

namespace {

constexpr uint16_t kSpecialRemoteGapMs = 260;

IRsendMeidi gRn02s13(kAcIrLedGpio);

// 把统一模式枚举映射成 RN02S13 驱动需要的命令编号。
int mideaMode(stdAc::opmode_t mode) {
  switch (mode) {
  case stdAc::opmode_t::kCool:
    return 1;
  case stdAc::opmode_t::kHeat:
    return 2;
  case stdAc::opmode_t::kDry:
    return 3;
  case stdAc::opmode_t::kFan:
    return 4;
  case stdAc::opmode_t::kAuto:
  default:
    return 0;
  }
}

// RN02S13 有自己的风速编号，和 stdAc::fanspeed_t 不完全一致。
int mideaFan(stdAc::fanspeed_t fan) {
  switch (fan) {
  case stdAc::fanspeed_t::kMin:
  case stdAc::fanspeed_t::kLow:
    return 1;
  case stdAc::fanspeed_t::kMedium:
    return 3;
  case stdAc::fanspeed_t::kHigh:
    return 4;
  case stdAc::fanspeed_t::kMax:
    return 5;
  case stdAc::fanspeed_t::kAuto:
  default:
    return 0;
  }
}

// 旧的本地驱动在使用前需要配置时序。
// 这些数值保留自已经验证可用的美的实现。
void beginRn02s13() {
  gRn02s13.begin_2();
  gRn02s13.setZBPL(40);
  gRn02s13.setCodeTime(500, 1600, 550, 4400, 4400, 5220);
}

// 完整状态同步用于添加空调流程和兼容旧版 AC 命令。
// RN02S13 每个属性都会发一条独立红外码，
// 所以详情控制应优先使用 sendRn02s13Action() 避免空调连续响多次。
bool sendRn02s13(const AcRemote &, const AcState &state) {
  if (!state.power) {
    gRn02s13.setPowers(false);
    return true;
  }

  gRn02s13.setPowers(true);
  delay(kSpecialRemoteGapMs);
  gRn02s13.setModes(mideaMode(state.mode));
  delay(kSpecialRemoteGapMs);
  gRn02s13.setFanSpeeds(mideaFan(state.fan));
  delay(kSpecialRemoteGapMs);
  gRn02s13.setTemps(state.temp);
  delay(kSpecialRemoteGapMs);

  if (state.swingv == stdAc::swingv_t::kAuto) {
    gRn02s13.setSwingUD(true);
    delay(kSpecialRemoteGapMs);
  }
  if (state.swingh == stdAc::swingh_t::kAuto) {
    gRn02s13.setSwingLR(true);
  }
  return true;
}

// 单项控制路径：一条串口命令只对应一条 RN02S13 红外命令。
bool sendRn02s13Action(const AcRemote &remote, const AcState &state,
                       AcAction action) {
  switch (action) {
  case AcAction::Power:
    gRn02s13.setPowers(state.power);
    return true;
  case AcAction::Temp:
    gRn02s13.setTemps(state.temp);
    return true;
  case AcAction::Mode:
    gRn02s13.setModes(mideaMode(state.mode));
    return true;
  case AcAction::Fan:
    gRn02s13.setFanSpeeds(mideaFan(state.fan));
    return true;
  case AcAction::SwingV:
    gRn02s13.setSwingUD(state.swingv == stdAc::swingv_t::kAuto);
    return true;
  case AcAction::SwingH:
    gRn02s13.setSwingLR(state.swingh == stdAc::swingh_t::kAuto);
    return true;
  case AcAction::State:
  default:
    return sendRn02s13(remote, state);
  }
}

const AcRemoteClass kMideaRn02s13Class = {
    "Midea RN02S13",
    beginRn02s13,
    sendRn02s13,
    sendRn02s13Action,
};

} // 命名空间

const AcRemote kMideaStandardRemote = {
    "midea_standard",
    "midea",
    "Midea standard",
    {17, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::MIDEA,
    kAcNoModel,
};

const AcRemote kMideaRn02s13Remote = {
    "midea_rn02s13",
    "midea",
    "Midea RN02S13",
    {17, 30, true, true, true},
    &kMideaRn02s13Class,
    decode_type_t::MIDEA,
    kAcNoModel,
};
