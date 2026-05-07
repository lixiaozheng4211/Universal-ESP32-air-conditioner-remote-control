#include "remotes/kelon_remotes.h"

#include "drivers/irac_backend.h"

// Kelon standard 已被 IRac 直接支持，所以纳入当前目录。
const AcRemote kKelonStandardRemote = {
    "kelon_standard",
    "kelon",
    "Kelon standard",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::KELON,
    kAcNoModel,
};
