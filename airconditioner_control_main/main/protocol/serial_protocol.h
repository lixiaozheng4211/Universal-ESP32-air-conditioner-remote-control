#pragma once

#include <Arduino.h>

// Line-based serial protocol used by Qt, Android, or a manual serial terminal.
// Each command is parsed synchronously and answered with exactly one OK/ERR
// result, except CATALOG which streams CAT lines between OK markers.
void serialProtocolPrintReady();
void serialProtocolProcessLine(String line);
