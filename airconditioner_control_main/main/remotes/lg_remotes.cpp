#include "remotes/lg_remotes.h"

#include "drivers/irac_backend.h"

// LG candidates are model numbers of the IRremoteESP8266 LG sender.
const AcRemote kLgGe6711Remote = {
    "lg_ge6711",
    "lg",
    "LG GE6711AR2853M",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::LG,
    static_cast<int16_t>(lg_ac_remote_model_t::GE6711AR2853M),
};

const AcRemote kLgAkb752Remote = {
    "lg_akb752",
    "lg",
    "LG AKB75215403",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::LG2,
    static_cast<int16_t>(lg_ac_remote_model_t::AKB75215403),
};

const AcRemote kLgAkb749Remote = {
    "lg_akb749",
    "lg",
    "LG AKB74955603",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::LG2,
    static_cast<int16_t>(lg_ac_remote_model_t::AKB74955603),
};

const AcRemote kLgAkb737Remote = {
    "lg_akb737",
    "lg",
    "LG AKB73757604",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::LG2,
    static_cast<int16_t>(lg_ac_remote_model_t::AKB73757604),
};

const AcRemote kLg6711Remote = {
    "lg_6711a20083v",
    "lg",
    "LG 6711A20083V",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::LG,
    static_cast<int16_t>(lg_ac_remote_model_t::LG6711A20083V),
};
