#pragma once

#include "ac_config.h"

#include <Arduino.h>
#include <IRac.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <stddef.h>
#include <stdint.h>

enum class AcNodeKind : uint8_t {
  Brand,
  Remote,
};

struct AcRemote;

struct AcState {
  bool power = true;
  stdAc::opmode_t mode = stdAc::opmode_t::kCool;
  float temp = 26.0f;
  stdAc::fanspeed_t fan = stdAc::fanspeed_t::kAuto;
  stdAc::swingv_t swingv = stdAc::swingv_t::kOff;
  stdAc::swingh_t swingh = stdAc::swingh_t::kOff;
  bool eco = false;
};

struct AcCapabilities {
  uint8_t minTemp;
  uint8_t maxTemp;
  bool supportsEco;
  bool supportsSwingV;
  bool supportsSwingH;
};

using AcBeginFn = void (*)();
using AcSendFn = bool (*)(const AcRemote &remote, const AcState &state);

struct AcRemoteClass {
  const char *name;
  AcBeginFn begin;
  AcSendFn send;
};

struct AcRemote {
  const char *id;
  const char *brandId;
  const char *name;
  AcCapabilities caps;
  const AcRemoteClass *klass;
  decode_type_t protocol;
  int16_t model;
};

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
