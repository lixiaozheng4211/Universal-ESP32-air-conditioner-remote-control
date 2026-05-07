#include "remotes/tcl_remotes.h"

#include "drivers/irac_backend.h"

// TCL 当前只保留 TAC09CHSD，因为这是这里能直接走 IRac 的型号。
const AcRemote kTclTac09chsdRemote = {
    "tcl_tac09chsd",
    "tcl",
    "TCL TAC09CHSD",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::TCL112AC,
    static_cast<int16_t>(tcl_ac_remote_model_t::TAC09CHSD),
};
