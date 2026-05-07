#include "drivers/irac_backend.h"

namespace {

IRac gIrac(kAcIrLedGpio);

// IRac 协议通常把完整空调状态编码进一帧。
// 精简协议之外的可选功能在这里统一关闭。
bool sendViaIRac(const AcRemote &remote, const AcState &state) {
  return gIrac.sendAc(remote.protocol, remote.model, state.power, state.mode,
                      state.temp, true, state.fan, state.swingv, state.swingh,
                      false, false, false, false, false, false, false);
}

// 对 IRac 后端来说，即使是单项 UI 操作也发送一帧完整状态。
// 这符合这类协议的编码方式，也避免为每个品牌重复写代码。
bool sendActionViaIRac(const AcRemote &remote, const AcState &state,
                       AcAction) {
  return sendViaIRac(remote, state);
}

} // 命名空间

const AcRemoteClass kIracRemoteClass = {
    "IRac",
    nullptr,
    sendViaIRac,
    sendActionViaIRac,
};
