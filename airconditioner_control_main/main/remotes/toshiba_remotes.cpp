#include "remotes/toshiba_remotes.h"

#include "drivers/irac_backend.h"

// Toshiba AC has a direct IRac sender and needs no special backend.
const AcRemote kToshibaAcRemote = {
    "toshiba_ac",
    "toshiba",
    "Toshiba AC",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::TOSHIBA_AC,
    kAcNoModel,
};
