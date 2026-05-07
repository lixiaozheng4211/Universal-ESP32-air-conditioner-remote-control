#pragma once

#include "ac_types.h"

// Midea remote candidates. RN02S13 uses the local ir_Midea2 special driver;
// the standard entry uses IRremoteESP8266's generic IRac backend.
extern const AcRemote kMideaStandardRemote;
extern const AcRemote kMideaRn02s13Remote;
