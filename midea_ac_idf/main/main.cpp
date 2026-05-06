#include "ac_config.h"
#include "air_conditioner.h"
#include "serial_protocol.h"

#include <Arduino.h>

void setup() {
  Serial.begin(kAcSerialBaud);
  delay(800);

  acBeginRemoteDrivers();
  serialProtocolPrintReady();
}

void loop() {
  if (Serial.available()) {
    serialProtocolProcessLine(Serial.readStringUntil('\n'));
  }

  delay(20);
}
