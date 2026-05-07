#pragma once

#include <stdint.h>

// 固件公共配置，串口协议和红外驱动都会使用这些常量。
// 只有硬件接线或波特率改变时才需要改这里。
constexpr uint8_t kAcIrLedGpio = 4;
constexpr uint32_t kAcSerialBaud = 115200;

// IRremoteESP8266 有些协议需要具体遥控器型号。
// 这个值表示当前协议不需要指定型号。
constexpr int16_t kAcNoModel = -1;
