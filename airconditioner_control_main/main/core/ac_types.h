#pragma once

#include "ac_config.h"

#include <Arduino.h>
#include <IRac.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <stddef.h>
#include <stdint.h>

// core 层只定义“空调控制”这个领域里的通用模型。
// 它不依赖 Qt/Android，也不把某个品牌的具体红外编码写死在这里，
// 这样串口协议、目录树和发送后端都能围绕同一套结构协作。

// 静态目录节点类型。品牌和遥控器候选放在同一个数组里，
// 是为了在 ESP32 侧避免 new/delete 和链表碎片，同时仍能表达“品牌 -> 多个候选遥控器”的树形关系。
enum class AcNodeKind : uint8_t {
  Brand,
  Remote,
};

// AC 动作表示本次要发送完整状态还是单项控制。
// State 用于添加空调和兼容旧命令；Power/Temp 等单项动作主要给详情控制界面使用。
// 这样 RN02S13 这种“一项功能发一条码”的遥控器不会因为一次调温连续发好几条红外。
enum class AcAction : uint8_t {
  State,
  Power,
  Temp,
  Mode,
  Fan,
  SwingV,
  SwingH,
};

struct AcRemote;

// 统一空调状态，IRac 后端和特殊品牌后端都使用这份结构。
// 这里只保留各品牌大多具备的公共控制项，像 ECO 这类品牌差异大的功能不放进通用协议，
// 避免上位机误以为所有空调都支持。
struct AcState {
  bool power = true;
  stdAc::opmode_t mode = stdAc::opmode_t::kCool;
  float temp = 26.0f;
  stdAc::fanspeed_t fan = stdAc::fanspeed_t::kAuto;
  stdAc::swingv_t swingv = stdAc::swingv_t::kOff;
  stdAc::swingh_t swingh = stdAc::swingh_t::kOff;
};

// 能力描述会通过 CATALOG 发给上位机，发送前也会再次校验，
// 这样 UI 能提前隐藏/禁用不支持的功能，固件侧也能兜底返回明确 ERR。
struct AcCapabilities {
  uint8_t minTemp;
  uint8_t maxTemp;
  bool supportsFan;
  bool supportsSwingV;
  bool supportsSwingH;
};

using AcBeginFn = void (*)();
using AcSendFn = bool (*)(const AcRemote &remote, const AcState &state);
using AcSendActionFn = bool (*)(const AcRemote &remote, const AcState &state,
                                AcAction action);

// C 风格多态函数表。不同遥控器可以共用 IRac 后端，
// 也可以接入 RN02S13 这种特殊后端；外层只拿 AcRemote 调 send。
// 这里不用 C++ 虚函数和动态对象，是为了让嵌入式侧的初始化和内存占用更可控。
struct AcRemoteClass {
  const char *name;
  AcBeginFn begin;
  AcSendFn send;
  AcSendActionFn sendAction;
};

// 一个遥控器候选。使用通用 IRac 后端时，
// protocol/model 会传给 IRremoteESP8266；使用特殊后端时，这两个字段可以只是目录描述。
// id 是上位机保存到本地库里的稳定标识，后续 Android/Qt 都靠它再次控制同一类遥控器。
struct AcRemote {
  const char *id;
  const char *brandId;
  const char *name;
  AcCapabilities caps;
  const AcRemoteClass *klass;
  decode_type_t protocol;
  int16_t model;
};

// 目录树节点使用数组下标而不是裸指针链接。
// parent/firstChild/nextSibling 相当于“父子链表”，但实际存储是静态数组。
// 这样既满足多叉树搜索结构，又方便 CATALOG 把同样的结构输出给上位机。
struct AcCatalogNode {
  const char *id;
  const char *name;
  AcNodeKind kind;
  int8_t parent;
  int8_t firstChild;
  int8_t nextSibling;
  const AcRemote *remote;
};

const char *acModeToString(stdAc::opmode_t mode);
const char *acActionToString(AcAction action);
