#pragma once

#include <stdint.h>

// 固件公共配置，串口协议和红外驱动都会使用这些常量。
// 只有硬件接线或波特率改变时才需要改这里。
constexpr uint8_t kAcIrLedGpio = 4;
constexpr uint32_t kAcSerialBaud = 115200;

// AcRemote 结构里统一保留 model 字段，供格力、海尔、富士通等协议区分具体遥控器型号。
// 但美的标准、AUX、Coolix 等协议没有型号分支，就统一填这个哨兵值表示“不需要指定 model”。
constexpr int16_t kAcNoModel = -1;
