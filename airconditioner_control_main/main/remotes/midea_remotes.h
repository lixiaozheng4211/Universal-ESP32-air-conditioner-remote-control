#pragma once

#include "ac_types.h"

// 美的遥控器候选。RN02S13 使用本地 ir_Midea2 特殊驱动；
// standard 条目使用 IRremoteESP8266 的通用 IRac 后端。
extern const AcRemote kMideaStandardRemote;
extern const AcRemote kMideaRn02s13Remote;
