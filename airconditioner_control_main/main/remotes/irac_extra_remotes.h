#pragma once

#include "ac_types.h"

// 这些遥控器来自 IRremoteESP8266 的 IRac::sendAc() 已有直发分支。
// 它们不需要单独写红外编码后端，只要给出 protocol/model 和能力描述，
// 就可以复用 kIracRemoteClass 发送完整状态帧。

extern const AcRemote kAirtonStandardRemote;
extern const AcRemote kAirwellStandardRemote;
extern const AcRemote kAmcorStandardRemote;
extern const AcRemote kArgoWrem2Remote;
extern const AcRemote kArgoWrem3Remote;
extern const AcRemote kBosch144Remote;
extern const AcRemote kCoolixStandardRemote;
extern const AcRemote kCoronaAcRemote;
extern const AcRemote kDelonghiAcRemote;
extern const AcRemote kEcoclimStandardRemote;
extern const AcRemote kEuromStandardRemote;
extern const AcRemote kFujitsuArRah2eRemote;
extern const AcRemote kFujitsuArDb1Remote;
extern const AcRemote kFujitsuArReb1eRemote;
extern const AcRemote kFujitsuArJw2Remote;
extern const AcRemote kFujitsuArRy4Remote;
extern const AcRemote kFujitsuArRew4eRemote;
extern const AcRemote kGoodweatherStandardRemote;
extern const AcRemote kKelvinatorStandardRemote;
extern const AcRemote kMirageKkg9ac1Remote;
extern const AcRemote kMirageKkg29ac1Remote;
extern const AcRemote kNeoclimaStandardRemote;
extern const AcRemote kRhossStandardRemote;
extern const AcRemote kSanyoAcRemote;
extern const AcRemote kSanyoAc88Remote;
extern const AcRemote kTeknopointGz055be1Remote;
extern const AcRemote kTechnibelAcRemote;
extern const AcRemote kTecoStandardRemote;
extern const AcRemote kTrotecStandardRemote;
extern const AcRemote kTrotec3550Remote;
extern const AcRemote kTrumaStandardRemote;
extern const AcRemote kVestelAcRemote;
extern const AcRemote kVoltas122lzfRemote;
extern const AcRemote kWhirlpoolDg11j13aRemote;
extern const AcRemote kWhirlpoolDg11j191Remote;
extern const AcRemote kTranscoldStandardRemote;
