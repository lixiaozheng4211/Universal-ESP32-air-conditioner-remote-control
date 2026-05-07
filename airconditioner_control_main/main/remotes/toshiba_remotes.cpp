#include "remotes/toshiba_remotes.h"

#include "drivers/irac_backend.h"

// Toshiba AC 有直接可用的 IRac 发送器，不需要特殊后端。
const AcRemote kToshibaAcRemote = {
    "toshiba_ac",
    "toshiba",
    "Toshiba AC",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::TOSHIBA_AC,
    kAcNoModel,
};
