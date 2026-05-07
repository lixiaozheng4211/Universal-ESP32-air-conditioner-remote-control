#include "remotes/sharp_remotes.h"

#include "drivers/irac_backend.h"

// Sharp entries are the IRac-supported A-series model variants.
const AcRemote kSharpA907Remote = {
    "sharp_a907",
    "sharp",
    "Sharp A907",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::SHARP_AC,
    static_cast<int16_t>(sharp_ac_remote_model_t::A907),
};

const AcRemote kSharpA705Remote = {
    "sharp_a705",
    "sharp",
    "Sharp A705",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::SHARP_AC,
    static_cast<int16_t>(sharp_ac_remote_model_t::A705),
};

const AcRemote kSharpA903Remote = {
    "sharp_a903",
    "sharp",
    "Sharp A903",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::SHARP_AC,
    static_cast<int16_t>(sharp_ac_remote_model_t::A903),
};
