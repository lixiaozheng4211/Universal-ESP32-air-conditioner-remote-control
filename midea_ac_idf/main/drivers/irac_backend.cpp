#include "drivers/irac_backend.h"

namespace {

IRac gIrac(kAcIrLedGpio);

bool sendViaIRac(const AcRemote &remote, const AcState &state) {
  return gIrac.sendAc(remote.protocol, remote.model, state.power, state.mode,
                      state.temp, true, state.fan, state.swingv, state.swingh,
                      false, false, state.eco, false, false, false, false);
}

bool sendActionViaIRac(const AcRemote &remote, const AcState &state,
                       AcAction) {
  return sendViaIRac(remote, state);
}

} // namespace

const AcRemoteClass kIracRemoteClass = {
    "IRac",
    nullptr,
    sendViaIRac,
    sendActionViaIRac,
};
