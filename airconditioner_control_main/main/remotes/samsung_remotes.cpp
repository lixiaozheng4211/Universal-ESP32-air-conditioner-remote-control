#include "remotes/samsung_remotes.h"

#include "drivers/irac_backend.h"

// 三星当前使用 IRac 中的通用 Samsung AC 发送器。
const AcRemote kSamsungAcRemote = {
    "samsung_ac",
    "samsung",
    "Samsung AC",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::SAMSUNG_AC,
    kAcNoModel,
};
