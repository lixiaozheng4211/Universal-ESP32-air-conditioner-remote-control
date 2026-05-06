#include "Arduino.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Midea.h>

const uint16_t kIrLed = 18;  // 发射引脚改为GPIO 18
IRMideaAC ac(kIrLed);

void printState() {
  Serial.println("MIDEA A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  uint64_t ir_code = ac.getRaw();
  Serial.printf("IR Code: 0x%012llX\n", (unsigned long long)ir_code);
}

extern "C" void app_main() {
  // 初始化 Arduino 环境
  initArduino();

  ac.begin();
  Serial.begin(115200);
  delay(200);

  Serial.println("Default state of the remote.");
  printState();
  Serial.println("Setting desired state for A/C.");
  
  ac.setFan(kMideaACFanAuto);
  ac.setMode(kMideaACCool);
  ac.setTemp(26);  // 17-30C
  ac.setSleep(false);

  bool power_state = true;

  while(true) {
    ac.setPower(power_state);
    if (power_state) {
      Serial.println("Sending IR command to Turn ON A/C ...");
    } else {
      Serial.println("Sending IR command to Turn OFF A/C ...");
    }
    
    ac.send();
    printState();
    
    power_state = !power_state;
    delay(5000);
  }
}
