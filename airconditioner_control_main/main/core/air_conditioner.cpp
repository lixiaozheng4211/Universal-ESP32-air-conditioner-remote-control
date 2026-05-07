#include "air_conditioner.h"

#include "ac_catalog.h"

namespace {

// Multiple catalog entries can share one backend class. Track initialized
// classes by pointer so begin() is called once per physical sender.
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

} // namespace

void acBeginRemoteDrivers() {
  const AcRemoteClass *initialized[16] = {};
  size_t initializedCount = 0;

  // Walk the catalog instead of hard-coding driver names; adding a remote only
  // requires declaring its AcRemote and linking it into the catalog array.
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

  // Default/off/auto values are allowed even when the remote does not expose
  // that capability, because they do not request an active unsupported feature.
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

  // Single-action commands are preferred when a backend supports them. The IRac
  // backend still sends a normal full-state frame because those protocols encode
  // the whole AC state in one transmission.
  if (ac.action != AcAction::State && ac.remote->klass->sendAction != nullptr) {
    return ac.remote->klass->sendAction(*ac.remote, ac.state, ac.action);
  }
  return ac.remote->klass->send(*ac.remote, ac.state);
}
