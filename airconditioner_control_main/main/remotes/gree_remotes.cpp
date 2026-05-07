#include "remotes/gree_remotes.h"

#include "drivers/irac_backend.h"

// Gree candidates are model variants of the same IRremoteESP8266 GREE sender.
const AcRemote kGreeYaw1fRemote = {
    "gree_yaw1f",
    "gree",
    "Gree YAW1F",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::GREE,
    static_cast<int16_t>(gree_ac_remote_model_t::YAW1F),
};

const AcRemote kGreeYbofbRemote = {
    "gree_ybofb",
    "gree",
    "Gree YBOFB",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::GREE,
    static_cast<int16_t>(gree_ac_remote_model_t::YBOFB),
};

const AcRemote kGreeYx1fsfRemote = {
    "gree_yx1fsf",
    "gree",
    "Gree YX1FSF",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::GREE,
    static_cast<int16_t>(gree_ac_remote_model_t::YX1FSF),
};
