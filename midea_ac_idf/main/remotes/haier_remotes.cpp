#include "remotes/haier_remotes.h"

#include "drivers/irac_backend.h"

const AcRemote kHaierAcRemote = {
    "haier_ac",
    "haier",
    "Haier HSU07",
    {16, 30, false, true, false},
    &kIracRemoteClass,
    decode_type_t::HAIER_AC,
    kAcNoModel,
};

const AcRemote kHaierAc160Remote = {
    "haier_ac160",
    "haier",
    "Haier AC160",
    {16, 30, false, true, false},
    &kIracRemoteClass,
    decode_type_t::HAIER_AC160,
    kAcNoModel,
};

const AcRemote kHaierAc176ARemote = {
    "haier_ac176_a",
    "haier",
    "Haier AC176 V9014557-A",
    {16, 30, false, true, true},
    &kIracRemoteClass,
    decode_type_t::HAIER_AC176,
    static_cast<int16_t>(haier_ac176_remote_model_t::V9014557_A),
};

const AcRemote kHaierAc176BRemote = {
    "haier_ac176_b",
    "haier",
    "Haier AC176 V9014557-B",
    {16, 30, false, true, true},
    &kIracRemoteClass,
    decode_type_t::HAIER_AC176,
    static_cast<int16_t>(haier_ac176_remote_model_t::V9014557_B),
};

const AcRemote kHaierYrw02Remote = {
    "haier_yrw02",
    "haier",
    "Haier YR-W02",
    {16, 30, false, true, false},
    &kIracRemoteClass,
    decode_type_t::HAIER_AC_YRW02,
    kAcNoModel,
};
