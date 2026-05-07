#pragma once

#include "ac_types.h"

// 奥克斯遥控器候选。IRremoteESP8266 把 AUX/YKR-T/011 放在 ELECTRA_AC 协议下，
// 因此这里直接复用通用 IRac 后端，不需要单独写品牌专用发送逻辑。
extern const AcRemote kAuxElectraRemote;
