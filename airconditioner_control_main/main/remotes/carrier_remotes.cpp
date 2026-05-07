#include "remotes/carrier_remotes.h"

#include "drivers/irac_backend.h"

// Carrier AC64 是当前目录中保留的 IRac 支持开利变体。
const AcRemote kCarrierAc64Remote = {
    "carrier_ac64",
    "carrier",
    "Carrier AC64",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::CARRIER_AC64,
    kAcNoModel,
};
