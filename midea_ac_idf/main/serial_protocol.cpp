#include "serial_protocol.h"

#include "ac_catalog.h"
#include "air_conditioner.h"

namespace {

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
    Serial.printf("CAT REMOTE id=%s brand=%s name=\"%s\" temp=%u-%u eco=%u "
                  "swingv=%u swingh=%u driver=\"%s\" next_sibling=%d\n",
                  remote.id, brand.id, remote.name, remote.caps.minTemp,
                  remote.caps.maxTemp, remote.caps.supportsEco ? 1 : 0,
                  remote.caps.supportsSwingV ? 1 : 0,
                  remote.caps.supportsSwingH ? 1 : 0,
                  remote.klass ? remote.klass->name : "none",
                  node.nextSibling);
  }
  ok("CATALOG END");
}

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
      if (!parseBoolValue(value, &ac->state.eco)) {
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

void handleAcCommand(const String &args) {
  AirConditioner ac;
  if (!parseAcArgs(args, &ac)) {
    return;
  }

  if (!acSend(ac)) {
    err("SEND_FAILED", ac.remote->id);
    return;
  }

  Serial.printf("OK SENT remote=%s power=%u mode=%s temp=%.1f\n",
                ac.remote->id, ac.state.power ? 1 : 0,
                acModeToString(ac.state.mode), ac.state.temp);
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

  if (command == "PING") {
    ok("PONG");
  } else if (command == "CATALOG") {
    printCatalog();
  } else if (command == "AC") {
    handleAcCommand(args);
  } else if (command == "HELP") {
    ok("COMMANDS PING CATALOG AC");
  } else {
    err("UNKNOWN_COMMAND", command.c_str());
  }
}
