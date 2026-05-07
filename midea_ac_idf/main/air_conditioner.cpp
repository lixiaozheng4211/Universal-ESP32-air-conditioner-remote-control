#include "air_conditioner.h"

#include "ac_catalog.h"

namespace {

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
  if (ac.state.eco && !ac.remote->caps.supportsEco) {
    return AcValidationError::UnsupportedEco;
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
  case AcValidationError::UnsupportedEco:
    return "UNSUPPORTED_ECO";
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
  case AcValidationError::UnsupportedEco:
    return "remote does not support eco";
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
  if (ac.action != AcAction::State && ac.remote->klass->sendAction != nullptr) {
    return ac.remote->klass->sendAction(*ac.remote, ac.state, ac.action);
  }
  return ac.remote->klass->send(*ac.remote, ac.state);
}
