#include "remotes/mitsubishi_remotes.h"

#include "drivers/irac_backend.h"

// Mitsubishi Electric and Heavy use different IR protocols, so both families
// are listed under one source module but separate brand ids in the catalog.
const AcRemote kMitsubishiAcRemote = {
    "mitsubishi_ac",
    "mitsubishi_electric",
    "Mitsubishi Electric AC",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::MITSUBISHI_AC,
    kAcNoModel,
};

const AcRemote kMitsubishi112Remote = {
    "mitsubishi112",
    "mitsubishi_electric",
    "Mitsubishi Electric 112",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::MITSUBISHI112,
    kAcNoModel,
};

const AcRemote kMitsubishi136Remote = {
    "mitsubishi136",
    "mitsubishi_electric",
    "Mitsubishi Electric 136",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::MITSUBISHI136,
    kAcNoModel,
};

const AcRemote kMitsubishiHeavy88Remote = {
    "mitsubishi_heavy_88",
    "mitsubishi_heavy",
    "Mitsubishi Heavy 88",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::MITSUBISHI_HEAVY_88,
    kAcNoModel,
};

const AcRemote kMitsubishiHeavy152Remote = {
    "mitsubishi_heavy_152",
    "mitsubishi_heavy",
    "Mitsubishi Heavy 152",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::MITSUBISHI_HEAVY_152,
    kAcNoModel,
};
