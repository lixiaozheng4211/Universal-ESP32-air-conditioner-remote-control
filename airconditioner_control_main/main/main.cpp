#include "ac_config.h"
#include "air_conditioner.h"
#include "drivers/ir_test.h"
#include "serial_protocol.h"

#include <Arduino.h>

// Firmware entry point. The board exposes a line-based serial protocol and
// uses the catalog/remote driver layer to turn each command into an IR frame.
void setup() {
  Serial.begin(kAcSerialBaud);
  delay(800);

  // Initialize each physical IR sender once, then announce that the protocol
  // is ready so host software can start its PING/CATALOG handshake.
  acBeginRemoteDrivers();
  irTestBegin();
  serialProtocolPrintReady();
}

void loop() {
  // Commands are newline-terminated. Keeping the loop small leaves timing work
  // inside the individual IR drivers where carrier generation belongs.
  if (Serial.available()) {
    serialProtocolProcessLine(Serial.readStringUntil('\n'));
  }
  delay(20);
}
