#pragma once

#include "ac_types.h"

// Generic backend built on IRremoteESP8266 IRac::sendAc(). Most supported
// protocols can be described with decode_type_t + model and share this class.
extern const AcRemoteClass kIracRemoteClass;
