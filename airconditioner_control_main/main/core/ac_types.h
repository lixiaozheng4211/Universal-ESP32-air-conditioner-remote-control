#pragma once

#include "ac_config.h"

#include <Arduino.h>
#include <IRac.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <stddef.h>
#include <stdint.h>

// Static catalog node type. Brands and remotes live in one array so the
// firmware can model a multi-child tree without heap allocation.
enum class AcNodeKind : uint8_t {
  Brand,
  Remote,
};

// AC action says how much of AcState should be sent. State is the compatible
// full-state command; the other values are single-control commands used by UI.
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

// Normalized AC state used by both IRac and special per-brand backends.
// Only common controls are kept here: power, mode, temp, fan and swing.
struct AcState {
  bool power = true;
  stdAc::opmode_t mode = stdAc::opmode_t::kCool;
  float temp = 26.0f;
  stdAc::fanspeed_t fan = stdAc::fanspeed_t::kAuto;
  stdAc::swingv_t swingv = stdAc::swingv_t::kOff;
  stdAc::swingh_t swingh = stdAc::swingh_t::kOff;
};

// Capability flags are reported to host software through CATALOG and checked
// before sending so unsupported controls fail with a clear ERR response.
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

// Function table for C-style polymorphism. A remote can use the generic IRac
// sender or a special backend while sharing the same AcRemote description.
struct AcRemoteClass {
  const char *name;
  AcBeginFn begin;
  AcSendFn send;
  AcSendActionFn sendAction;
};

// One candidate remote control. protocol/model are passed to IRremoteESP8266
// when the generic IRac backend is used.
struct AcRemote {
  const char *id;
  const char *brandId;
  const char *name;
  AcCapabilities caps;
  const AcRemoteClass *klass;
  decode_type_t protocol;
  int16_t model;
};

// Tree node stored as array indexes instead of pointers. This keeps traversal
// simple for Qt/Android and avoids dynamic memory on the ESP32.
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
