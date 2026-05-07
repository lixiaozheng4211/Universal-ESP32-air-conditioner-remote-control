#include "remotes/kelon_remotes.h"

#include "drivers/irac_backend.h"

// Kelon standard is included because it is directly supported by IRac.
const AcRemote kKelonStandardRemote = {
    "kelon_standard",
    "kelon",
    "Kelon standard",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::KELON,
    kAcNoModel,
};
