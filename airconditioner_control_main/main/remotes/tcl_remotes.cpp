#include "remotes/tcl_remotes.h"

#include "drivers/irac_backend.h"

// TCL is kept to the TAC09CHSD model because it is the direct IRac sender here.
const AcRemote kTclTac09chsdRemote = {
    "tcl_tac09chsd",
    "tcl",
    "TCL TAC09CHSD",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::TCL112AC,
    static_cast<int16_t>(tcl_ac_remote_model_t::TAC09CHSD),
};
