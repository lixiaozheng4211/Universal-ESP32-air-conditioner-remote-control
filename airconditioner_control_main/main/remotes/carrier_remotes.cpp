#include "remotes/carrier_remotes.h"

#include "drivers/irac_backend.h"

// Carrier AC64 is the IRac-supported Carrier variant kept in this catalog.
const AcRemote kCarrierAc64Remote = {
    "carrier_ac64",
    "carrier",
    "Carrier AC64",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::CARRIER_AC64,
    kAcNoModel,
};
