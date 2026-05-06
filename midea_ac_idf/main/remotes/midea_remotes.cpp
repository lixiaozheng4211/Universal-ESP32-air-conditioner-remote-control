#include "remotes/midea_remotes.h"

#include "drivers/irac_backend.h"
#include "ir_Midea2.h"

namespace {

constexpr uint16_t kSpecialRemoteGapMs = 260;

IRsendMeidi gRn02s13(kAcIrLedGpio);

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

void beginRn02s13() {
  gRn02s13.begin_2();
  gRn02s13.setZBPL(40);
  gRn02s13.setCodeTime(500, 1600, 550, 4400, 4400, 5220);
}

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

  if (state.eco) {
    gRn02s13.setEco(true);
    delay(kSpecialRemoteGapMs);
  }
  if (state.swingv == stdAc::swingv_t::kAuto) {
    gRn02s13.setSwingUD(true);
    delay(kSpecialRemoteGapMs);
  }
  if (state.swingh == stdAc::swingh_t::kAuto) {
    gRn02s13.setSwingLR(true);
  }
  return true;
}

const AcRemoteClass kMideaRn02s13Class = {
    "Midea RN02S13",
    beginRn02s13,
    sendRn02s13,
};

} // namespace

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
