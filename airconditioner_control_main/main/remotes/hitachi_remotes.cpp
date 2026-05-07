#include "remotes/hitachi_remotes.h"

#include "drivers/irac_backend.h"

// 日立条目覆盖 IRac 已支持的帧族和型号变体。
const AcRemote kHitachiAcRemote = {
    "hitachi_ac",
    "hitachi",
    "Hitachi AC",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::HITACHI_AC,
    kAcNoModel,
};

const AcRemote kHitachiAc1ARemote = {
    "hitachi_ac1_a",
    "hitachi",
    "Hitachi AC1 R-LT0541-HTA A",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::HITACHI_AC1,
    static_cast<int16_t>(hitachi_ac1_remote_model_t::R_LT0541_HTA_A),
};

const AcRemote kHitachiAc1BRemote = {
    "hitachi_ac1_b",
    "hitachi",
    "Hitachi AC1 R-LT0541-HTA B",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::HITACHI_AC1,
    static_cast<int16_t>(hitachi_ac1_remote_model_t::R_LT0541_HTA_B),
};

const AcRemote kHitachiAc264Remote = {
    "hitachi_ac264",
    "hitachi",
    "Hitachi AC264",
    {16, 30, true, false, false},
    &kIracRemoteClass,
    decode_type_t::HITACHI_AC264,
    kAcNoModel,
};

const AcRemote kHitachiAc296Remote = {
    "hitachi_ac296",
    "hitachi",
    "Hitachi AC296",
    {16, 30, true, false, false},
    &kIracRemoteClass,
    decode_type_t::HITACHI_AC296,
    kAcNoModel,
};

const AcRemote kHitachiAc344Remote = {
    "hitachi_ac344",
    "hitachi",
    "Hitachi AC344",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::HITACHI_AC344,
    kAcNoModel,
};

const AcRemote kHitachiAc424Remote = {
    "hitachi_ac424",
    "hitachi",
    "Hitachi AC424",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::HITACHI_AC424,
    kAcNoModel,
};
