#include "remotes/aux_remotes.h"

#include "drivers/irac_backend.h"

// AUX KFR-35GW / YKR-T/011 在 IRremoteESP8266 中由 ELECTRA_AC 发送器覆盖。
const AcRemote kAuxElectraRemote = {
    "aux_electra",
    "aux",
    "AUX YKR-T/011",
    {16, 32, true, true, true},
    &kIracRemoteClass,
    decode_type_t::ELECTRA_AC,
    kAcNoModel,
};
