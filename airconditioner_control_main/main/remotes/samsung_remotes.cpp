#include "remotes/samsung_remotes.h"

#include "drivers/irac_backend.h"

// Samsung is currently represented by the generic IRac Samsung AC sender.
const AcRemote kSamsungAcRemote = {
    "samsung_ac",
    "samsung",
    "Samsung AC",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::SAMSUNG_AC,
    kAcNoModel,
};
