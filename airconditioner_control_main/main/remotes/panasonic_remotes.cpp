#include "remotes/panasonic_remotes.h"

#include "drivers/irac_backend.h"

// Panasonic models map to IRremoteESP8266 panasonic_ac_remote_model_t values.
const AcRemote kPanasonicLkeRemote = {
    "panasonic_lke",
    "panasonic",
    "Panasonic LKE",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::PANASONIC_AC,
    static_cast<int16_t>(panasonic_ac_remote_model_t::kPanasonicLke),
};

const AcRemote kPanasonicNkeRemote = {
    "panasonic_nke",
    "panasonic",
    "Panasonic NKE",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::PANASONIC_AC,
    static_cast<int16_t>(panasonic_ac_remote_model_t::kPanasonicNke),
};

const AcRemote kPanasonicDkeRemote = {
    "panasonic_dke",
    "panasonic",
    "Panasonic DKE/PKR",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::PANASONIC_AC,
    static_cast<int16_t>(panasonic_ac_remote_model_t::kPanasonicDke),
};

const AcRemote kPanasonicJkeRemote = {
    "panasonic_jke",
    "panasonic",
    "Panasonic JKE",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::PANASONIC_AC,
    static_cast<int16_t>(panasonic_ac_remote_model_t::kPanasonicJke),
};

const AcRemote kPanasonicCkpRemote = {
    "panasonic_ckp",
    "panasonic",
    "Panasonic CKP",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::PANASONIC_AC,
    static_cast<int16_t>(panasonic_ac_remote_model_t::kPanasonicCkp),
};

const AcRemote kPanasonicRkrRemote = {
    "panasonic_rkr",
    "panasonic",
    "Panasonic RKR",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::PANASONIC_AC,
    static_cast<int16_t>(panasonic_ac_remote_model_t::kPanasonicRkr),
};

const AcRemote kPanasonicAc32Remote = {
    "panasonic_ac32",
    "panasonic",
    "Panasonic AC32",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::PANASONIC_AC32,
    kAcNoModel,
};
