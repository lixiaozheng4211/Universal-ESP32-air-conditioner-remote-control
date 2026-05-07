#include "ac_config.h"
#include "air_conditioner.h"
#include "drivers/ir_test.h"
#include "serial_protocol.h"

#include <Arduino.h>

// 固件入口：程序本身只负责“串口收命令 -> 目录选遥控器 -> 驱动发红外”。
// 具体品牌协议、命令解析和红外测试都拆到独立模块，避免 main.cpp 变成大杂烩。
void setup() {
  Serial.begin(kAcSerialBaud);
  delay(800);

  // 启动时先初始化所有会用到的红外发送后端，再打印 READY。
  // 上位机看到 READY 或 PING 成功后，就可以继续请求 CATALOG 并开始控制。
  acBeginRemoteDrivers();
  irTestBegin();
  serialProtocolPrintReady();
}

void loop() {
  // 串口协议按行处理，每条命令以 '\n' 结束。
  // 主循环保持简单，是为了把红外载波、品牌协议时序这些事情留给驱动层。
  if (Serial.available()) {
    serialProtocolProcessLine(Serial.readStringUntil('\n'));
  }
  delay(20);
}
