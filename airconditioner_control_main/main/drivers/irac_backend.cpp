#include "drivers/irac_backend.h"

namespace {

IRac gIrac(kAcIrLedGpio);

// IRac protocols encode a complete AC state into one frame. Optional features
// outside the simplified protocol are kept disabled here.
bool sendViaIRac(const AcRemote &remote, const AcState &state) {
  return gIrac.sendAc(remote.protocol, remote.model, state.power, state.mode,
                      state.temp, true, state.fan, state.swingv, state.swingh,
                      false, false, false, false, false, false, false);
}

// For IRac-backed remotes, a single UI action still becomes one complete IRac
// frame. This is correct for these protocols and avoids per-brand duplicate code.
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
