#pragma once

#include "ac_types.h"

// 这一层负责把“已经解析好的空调命令”变成一次安全的发送动作。
// 串口层只管把文本变成结构体，遥控器后端只管真正发红外，
// 中间的校验和分发放在这里，方便以后增加 Bluetooth/Android 直连等其它入口。

// 一条 AC 串口命令解析后的运行时对象。
// remote 负责携带后端函数表，action 决定完整发送还是单项发送。
struct AirConditioner {
  const AcRemote *remote = nullptr;
  AcAction action = AcAction::State;
  AcState state;
};

// 校验错误独立于串口解析，保证 Qt、Android 和手动串口命令
// 都会走同一套能力检查，不会出现某个入口绕过限制直接发不支持的功能。
enum class AcValidationError : uint8_t {
  Ok,
  MissingRemote,
  BadTemp,
  UnsupportedFan,
  UnsupportedSwingV,
  UnsupportedSwingH,
  MissingDriver,
};

// 初始化目录中出现过的所有遥控器驱动类，重复使用的后端只初始化一次。
void acBeginRemoteDrivers();

// 发送前检查命令是否符合所选遥控器的能力范围。
AcValidationError acValidate(const AirConditioner &ac);
const char *acValidationCode(AcValidationError error);
const char *acValidationMessage(AcValidationError error);

// 根据所选遥控器分发到对应后端。
// 特殊遥控器可以实现 sendAction，避免一次 UI 操作发出多条红外码；
// 普通 IRac 遥控器则继续发送一帧完整状态。
bool acSend(const AirConditioner &ac);
