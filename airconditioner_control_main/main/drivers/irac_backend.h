#pragma once

#include "ac_types.h"

// 基于 IRremoteESP8266 IRac::sendAc() 的通用后端。
// 大多数受支持协议都可以用 decode_type_t + model 描述，并共用这个类。
extern const AcRemoteClass kIracRemoteClass;
