#include "serial_protocol.h"

#include "ac_catalog.h"
#include "air_conditioner.h"
#include "drivers/ir_test.h"

#include <stdlib.h>

namespace {

// Response helpers keep every host-facing line machine-readable.
void ok(const char *message) { Serial.printf("OK %s\n", message); }

void err(const char *code, const char *message) {
  Serial.printf("ERR %s %s\n", code, message);
}

bool parseBoolValue(const String &value, bool *out) {
  if (value == "1" || value == "true" || value == "on") {
    *out = true;
    return true;
  }
  if (value == "0" || value == "false" || value == "off") {
    *out = false;
    return true;
  }
  return false;
}

bool parseMode(const String &value, stdAc::opmode_t *out) {
  if (value == "auto") {
    *out = stdAc::opmode_t::kAuto;
  } else if (value == "cool") {
    *out = stdAc::opmode_t::kCool;
  } else if (value == "heat") {
    *out = stdAc::opmode_t::kHeat;
  } else if (value == "dry") {
    *out = stdAc::opmode_t::kDry;
  } else if (value == "fan") {
    *out = stdAc::opmode_t::kFan;
  } else {
    return false;
  }
  return true;
}

bool parseFan(const String &value, stdAc::fanspeed_t *out) {
  if (value == "auto") {
    *out = stdAc::fanspeed_t::kAuto;
  } else if (value == "min") {
    *out = stdAc::fanspeed_t::kMin;
  } else if (value == "low") {
    *out = stdAc::fanspeed_t::kLow;
  } else if (value == "med" || value == "medium") {
    *out = stdAc::fanspeed_t::kMedium;
  } else if (value == "high") {
    *out = stdAc::fanspeed_t::kHigh;
  } else if (value == "max") {
    *out = stdAc::fanspeed_t::kMax;
  } else {
    return false;
  }
  return true;
}

bool parseSwingV(const String &value, stdAc::swingv_t *out) {
  if (value == "off") {
    *out = stdAc::swingv_t::kOff;
  } else if (value == "auto" || value == "on") {
    *out = stdAc::swingv_t::kAuto;
  } else {
    return false;
  }
  return true;
}

bool parseSwingH(const String &value, stdAc::swingh_t *out) {
  if (value == "off") {
    *out = stdAc::swingh_t::kOff;
  } else if (value == "auto" || value == "on") {
    *out = stdAc::swingh_t::kAuto;
  } else {
    return false;
  }
  return true;
}

bool parseAction(const String &value, AcAction *out) {
  if (value == "state") {
    *out = AcAction::State;
  } else if (value == "power") {
    *out = AcAction::Power;
  } else if (value == "temp") {
    *out = AcAction::Temp;
  } else if (value == "mode") {
    *out = AcAction::Mode;
  } else if (value == "fan") {
    *out = AcAction::Fan;
  } else if (value == "swingv") {
    *out = AcAction::SwingV;
  } else if (value == "swingh") {
    *out = AcAction::SwingH;
  } else {
    return false;
  }
  return true;
}

// Shared integer parser for IRTEST options. min/max live at each call site so
// the error message can name the accepted range.
bool parseUnsignedValue(const String &value, uint32_t minValue,
                        uint32_t maxValue, uint32_t *out) {
  char *end = nullptr;
  const unsigned long parsed = strtoul(value.c_str(), &end, 0);
  if (end == value.c_str() || *end != '\0' || parsed < minValue ||
      parsed > maxValue) {
    return false;
  }
  *out = static_cast<uint32_t>(parsed);
  return true;
}

// Accept both human-friendly values like 38k/40khz and raw Hz values.
bool parseFrequency(String value, uint32_t *out) {
  value.trim();
  value.toLowerCase();
  if (value.endsWith("khz")) {
    value.remove(value.length() - 3);
  } else if (value.endsWith("k")) {
    value.remove(value.length() - 1);
  }
  value.trim();

  uint32_t freq = 0;
  if (!parseUnsignedValue(value, 30, 60000, &freq)) {
    return false;
  }
  if (freq < 1000) {
    freq *= 1000;
  }
  if (freq < 30000 || freq > 60000) {
    return false;
  }
  *out = freq;
  return true;
}

bool parseDataValue(const String &value, uint64_t *out) {
  char *end = nullptr;
  const unsigned long long parsed = strtoull(value.c_str(), &end, 0);
  if (end == value.c_str() || *end != '\0') {
    return false;
  }
  *out = static_cast<uint64_t>(parsed);
  return true;
}

// CATALOG mirrors the static tree. Qt can build its brand/remote view directly
// from these lines, and Android can store a chosen remote id for later control.
void printCatalog() {
  Serial.printf("OK CATALOG remotes=%u\n", acRemoteCount());
  for (size_t i = 0; i < acCatalogNodeCount(); ++i) {
    const AcCatalogNode &node = acCatalogNodeAt(i);
    if (node.kind == AcNodeKind::Brand) {
      Serial.printf("CAT BRAND id=%s name=\"%s\" first_child=%d "
                    "next_sibling=%d\n",
                    node.id, node.name, node.firstChild, node.nextSibling);
      continue;
    }

    const AcCatalogNode &brand = acCatalogNodeAt(node.parent);
    const AcRemote &remote = *node.remote;
    Serial.printf("CAT REMOTE id=%s brand=%s name=\"%s\" temp=%u-%u fan=%u "
                  "swingv=%u swingh=%u driver=\"%s\" next_sibling=%d\n",
                  remote.id, brand.id, remote.name, remote.caps.minTemp,
                  remote.caps.maxTemp, remote.caps.supportsFan ? 1 : 0,
                  remote.caps.supportsSwingV ? 1 : 0,
                  remote.caps.supportsSwingH ? 1 : 0,
                  remote.klass ? remote.klass->name : "none",
                  node.nextSibling);
  }
  ok("CATALOG END");
}

// Parse AC key=value pairs. Unknown keys are rejected, while eco is accepted
// and ignored for one compatibility version because older Qt builds sent it.
bool parseAcArgs(String args, AirConditioner *ac) {
  args.trim();
  int start = 0;
  while (start < args.length()) {
    while (start < args.length() && args[start] == ' ') {
      start++;
    }
    if (start >= args.length()) {
      break;
    }

    int end = args.indexOf(' ', start);
    if (end < 0) {
      end = args.length();
    }

    String token = args.substring(start, end);
    int eq = token.indexOf('=');
    if (eq <= 0 || eq == token.length() - 1) {
      err("BAD_TOKEN", "expected key=value");
      return false;
    }

    String key = token.substring(0, eq);
    String value = token.substring(eq + 1);
    key.toLowerCase();
    value.toLowerCase();

    if (key == "remote") {
      ac->remote = acFindRemoteById(value);
      if (ac->remote == nullptr) {
        err("UNKNOWN_REMOTE", value.c_str());
        return false;
      }
    } else if (key == "action") {
      if (!parseAction(value, &ac->action)) {
        err("BAD_ACTION", "use state/power/temp/mode/fan/swingv/swingh");
        return false;
      }
    } else if (key == "power") {
      if (!parseBoolValue(value, &ac->state.power)) {
        err("BAD_POWER", "use 0/1");
        return false;
      }
    } else if (key == "mode") {
      if (!parseMode(value, &ac->state.mode)) {
        err("BAD_MODE", "use auto/cool/heat/dry/fan");
        return false;
      }
    } else if (key == "temp") {
      ac->state.temp = value.toFloat();
    } else if (key == "fan") {
      if (!parseFan(value, &ac->state.fan)) {
        err("BAD_FAN", "use auto/low/med/high/max");
        return false;
      }
    } else if (key == "swingv") {
      if (!parseSwingV(value, &ac->state.swingv)) {
        err("BAD_SWINGV", "use off/auto");
        return false;
      }
    } else if (key == "swingh") {
      if (!parseSwingH(value, &ac->state.swingh)) {
        err("BAD_SWINGH", "use off/auto");
        return false;
      }
    } else if (key == "eco") {
      bool ignoredEco = false;
      if (!parseBoolValue(value, &ignoredEco)) {
        err("BAD_ECO", "use 0/1");
        return false;
      }
    } else {
      err("UNKNOWN_KEY", key.c_str());
      return false;
    }

    start = end + 1;
  }

  const AcValidationError validation = acValidate(*ac);
  if (validation != AcValidationError::Ok) {
    err(acValidationCode(validation), acValidationMessage(validation));
    return false;
  }
  return true;
}

// IRTEST is intentionally separate from AC control. It helps verify GPIO,
// carrier frequency, duty cycle, and basic receiver visibility during wiring.
bool parseIrTestArgs(String args, IrTestRequest *request) {
  args.trim();
  int start = 0;
  while (start < args.length()) {
    while (start < args.length() && args[start] == ' ') {
      start++;
    }
    if (start >= args.length()) {
      break;
    }

    int end = args.indexOf(' ', start);
    if (end < 0) {
      end = args.length();
    }

    String token = args.substring(start, end);
    int eq = token.indexOf('=');
    if (eq <= 0 || eq == token.length() - 1) {
      err("BAD_TOKEN", "expected key=value");
      return false;
    }

    String key = token.substring(0, eq);
    String value = token.substring(eq + 1);
    key.toLowerCase();
    value.toLowerCase();

    if (key == "freq" || key == "frequency") {
      if (!parseFrequency(value, &request->freqHz)) {
        err("BAD_FREQ", "use 38/40/38000/40000");
        return false;
      }
    } else if (key == "count" || key == "repeat") {
      uint32_t count = 0;
      if (!parseUnsignedValue(value, 1, 20, &count)) {
        err("BAD_COUNT", "use 1..20");
        return false;
      }
      request->count = static_cast<uint16_t>(count);
    } else if (key == "ms" || key == "carrier_ms") {
      uint32_t carrierMs = 0;
      if (!parseUnsignedValue(value, 1, 2000, &carrierMs)) {
        err("BAD_MS", "use 1..2000");
        return false;
      }
      request->carrierMs = static_cast<uint16_t>(carrierMs);
    } else if (key == "duty") {
      uint32_t duty = 0;
      if (!parseUnsignedValue(value, 10, 80, &duty)) {
        err("BAD_DUTY", "use 10..80");
        return false;
      }
      request->dutyPercent = static_cast<uint8_t>(duty);
    } else if (key == "data") {
      if (!parseDataValue(value, &request->data)) {
        err("BAD_DATA", "use decimal or 0x hex");
        return false;
      }
    } else if (key == "mode" || key == "type") {
      if (value == "nec" || value == "frame") {
        request->mode = IrTestMode::Nec;
      } else if (value == "carrier" || value == "raw") {
        request->mode = IrTestMode::Carrier;
      } else {
        err("BAD_MODE", "use nec/carrier");
        return false;
      }
    } else {
      err("UNKNOWN_KEY", key.c_str());
      return false;
    }

    start = end + 1;
  }
  return true;
}

void handleAcCommand(const String &args) {
  AirConditioner ac;
  if (!parseAcArgs(args, &ac)) {
    return;
  }

  if (!acSend(ac)) {
    err("SEND_FAILED", ac.remote->id);
    return;
  }

  Serial.printf("OK SENT remote=%s action=%s power=%u mode=%s temp=%.1f\n",
                ac.remote->id, acActionToString(ac.action),
                ac.state.power ? 1 : 0,
                acModeToString(ac.state.mode), ac.state.temp);
}

// Send a direct IR test frame/carrier and return the exact settings used.
void handleIrTestCommand(const String &args) {
  IrTestRequest request;
  if (!parseIrTestArgs(args, &request)) {
    return;
  }

  if (!irTestSend(request)) {
    err("BAD_IRTEST", "check freq/count/ms/duty");
    return;
  }

  Serial.printf("OK IRTEST mode=%s freq=%lu count=%u duty=%u ms=%u "
                "data=0x%08llX\n",
                irTestModeToString(request.mode),
                static_cast<unsigned long>(request.freqHz), request.count,
                request.dutyPercent, request.carrierMs,
                static_cast<unsigned long long>(request.data));
}

} // namespace

void serialProtocolPrintReady() {
  Serial.printf("OK READY protocol=1 baud=%lu ir_gpio=%u\n",
                static_cast<unsigned long>(kAcSerialBaud), kAcIrLedGpio);
}

void serialProtocolProcessLine(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }

  int space = line.indexOf(' ');
  String command = space < 0 ? line : line.substring(0, space);
  String args = space < 0 ? "" : line.substring(space + 1);
  command.toUpperCase();

  // Keep command names uppercase and arguments lowercase key=value text so the
  // protocol stays simple for Android USB-serial libraries.
  if (command == "PING") {
    ok("PONG");
  } else if (command == "CATALOG") {
    printCatalog();
  } else if (command == "AC") {
    handleAcCommand(args);
  } else if (command == "IRTEST") {
    handleIrTestCommand(args);
  } else if (command == "HELP") {
    ok("COMMANDS PING CATALOG AC(action=state/power/temp/mode/fan/swingv/swingh) IRTEST");
  } else {
    err("UNKNOWN_COMMAND", command.c_str());
  }
}
