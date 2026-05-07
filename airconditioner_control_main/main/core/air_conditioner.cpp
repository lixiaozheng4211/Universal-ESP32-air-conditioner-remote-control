#include "air_conditioner.h"

#include "ac_catalog.h"

namespace {

// 多个目录项可能共用同一个后端类。这里按类指针去重，
// 确保每个物理红外发送驱动只初始化一次。
// 例如很多品牌都走 kIracRemoteClass，重复 begin() 没意义，也可能打乱底层发送状态。
bool classAlreadySeen(const AcRemoteClass *klass,
                      const AcRemoteClass *const *classes,
                      size_t classCount) {
  for (size_t i = 0; i < classCount; ++i) {
    if (classes[i] == klass) {
      return true;
    }
  }
  return false;
}

} // 命名空间

void acBeginRemoteDrivers() {
  const AcRemoteClass *initialized[16] = {};
  size_t initializedCount = 0;

  // 遍历目录而不是硬编码驱动名。
  // 这样新增遥控器时只需要声明 AcRemote 并挂到目录数组里，
  // 初始化逻辑会自动发现它需要哪个后端，main.cpp 不需要跟着改。
  for (size_t i = 0; i < acCatalogNodeCount(); ++i) {
    const AcCatalogNode &node = acCatalogNodeAt(i);
    if (node.kind != AcNodeKind::Remote || node.remote == nullptr ||
        node.remote->klass == nullptr || node.remote->klass->begin == nullptr) {
      continue;
    }

    const AcRemoteClass *klass = node.remote->klass;
    if (classAlreadySeen(klass, initialized, initializedCount)) {
      continue;
    }

    klass->begin();
    if (initializedCount < sizeof(initialized) / sizeof(initialized[0])) {
      initialized[initializedCount++] = klass;
    }
  }
}

AcValidationError acValidate(const AirConditioner &ac) {
  // 先检查 remote 和 driver，是为了让串口层能返回更准确的错误：
  // 没选遥控器、遥控器 id 错误、驱动未挂接，这些都不是红外发送失败。
  if (ac.remote == nullptr) {
    return AcValidationError::MissingRemote;
  }
  if (ac.remote->klass == nullptr || ac.remote->klass->send == nullptr) {
    return AcValidationError::MissingDriver;
  }
  if (ac.state.temp < ac.remote->caps.minTemp ||
      ac.state.temp > ac.remote->caps.maxTemp) {
    return AcValidationError::BadTemp;
  }

  // 默认、关闭、自动这类“无额外动作”的值允许通过。
  // 这样不支持左右风的遥控器仍然可以接收 swingh=off；
  // 只有真正请求 swingh=auto 这类主动功能时才返回错误。
  if (ac.state.fan != stdAc::fanspeed_t::kAuto &&
      !ac.remote->caps.supportsFan) {
    return AcValidationError::UnsupportedFan;
  }
  if (ac.state.swingv != stdAc::swingv_t::kOff &&
      !ac.remote->caps.supportsSwingV) {
    return AcValidationError::UnsupportedSwingV;
  }
  if (ac.state.swingh != stdAc::swingh_t::kOff &&
      !ac.remote->caps.supportsSwingH) {
    return AcValidationError::UnsupportedSwingH;
  }
  return AcValidationError::Ok;
}

const char *acValidationCode(AcValidationError error) {
  switch (error) {
  case AcValidationError::Ok:
    return "OK";
  case AcValidationError::MissingRemote:
    return "MISSING_REMOTE";
  case AcValidationError::BadTemp:
    return "BAD_TEMP";
  case AcValidationError::UnsupportedFan:
    return "UNSUPPORTED_FAN";
  case AcValidationError::UnsupportedSwingV:
    return "UNSUPPORTED_SWINGV";
  case AcValidationError::UnsupportedSwingH:
    return "UNSUPPORTED_SWINGH";
  case AcValidationError::MissingDriver:
    return "MISSING_DRIVER";
  default:
    return "BAD_AC";
  }
}

const char *acValidationMessage(AcValidationError error) {
  switch (error) {
  case AcValidationError::Ok:
    return "ready";
  case AcValidationError::MissingRemote:
    return "AC command requires remote=<id>";
  case AcValidationError::BadTemp:
    return "temperature out of remote range";
  case AcValidationError::UnsupportedFan:
    return "remote does not support fan speed";
  case AcValidationError::UnsupportedSwingV:
    return "remote does not support vertical swing";
  case AcValidationError::UnsupportedSwingH:
    return "remote does not support horizontal swing";
  case AcValidationError::MissingDriver:
    return "remote driver is missing";
  default:
    return "invalid AC request";
  }
}

bool acSend(const AirConditioner &ac) {
  if (acValidate(ac) != AcValidationError::Ok) {
    return false;
  }

  // 如果后端支持单项动作，优先走 sendAction。
  // 这个分支主要解决 RN02S13 调温连续响多次的问题；
  // IRac 后端仍会发完整状态帧，因为这些协议本来就是一帧编码完整状态。
  if (ac.action != AcAction::State && ac.remote->klass->sendAction != nullptr) {
    return ac.remote->klass->sendAction(*ac.remote, ac.state, ac.action);
  }
  return ac.remote->klass->send(*ac.remote, ac.state);
}
