#include "serial_protocol.h"

#include "ac_catalog.h"
#include "air_conditioner.h"
#include "drivers/ir_test.h"

#include <stdlib.h>

namespace {

// 响应辅助函数，保证所有发给上位机的行都便于机器解析。
// 统一 OK/ERR 前缀后，Qt/Android 只需要按行判断前缀即可做状态机。
void ok(const char *message) { Serial.printf("OK %s\n", message); }

void err(const char *code, const char *message) {
  Serial.printf("ERR %s %s\n", code, message);
}

bool parseBoolValue(const String &value, bool *out) {
  // 同时接受 1/0、true/false、on/off，是为了兼容手动调试习惯；
  // 文档里仍推荐 Android 端固定使用 1/0，减少歧义。
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
  // 协议层使用简单英文枚举，内部立即转换成 IRremoteESP8266 的 stdAc 类型。
  // 这样后端不需要再关心串口文本格式。
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
  // action 是 v1.0 的关键扩展：state 代表完整状态同步，
  // 其它值代表只发某个被用户改动的控制项。
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

// IRTEST 选项共用的整数解析函数。
// min/max 放在调用点，是因为 freq/count/ms/duty 的合法范围不同；
// 共用解析逻辑可以减少重复代码，但错误提示仍能保持具体。
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

// 同时接受 38k、40khz 这类人工输入和原始 Hz 数值。
// 这样现场排查 38K/40K 载波时，不需要记住必须输入 38000 还是 38。
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

// CATALOG 输出静态目录树。这里没有直接输出 JSON，
// 是为了保持协议“逐行可读、可用串口助手复制测试”的特点。
// Qt 可以用 CAT BRAND/CAT REMOTE 构建树形视图，
// Android 端也可以保存用户选中的 remote id 供后续控制使用。
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

// 解析 AC 命令的 key=value 参数。
// 这里选择严格拒绝未知字段，是为了让 Android/Qt 在开发阶段尽早发现拼写错误；
// 否则一个写错的参数会被静默忽略，表现就像空调“不响应”。
// eco 例外：它是旧版协议暴露过的字段，为兼容旧客户端保留一版，只解析并忽略。
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
      // temp 先转成数值，范围统一交给 acValidate() 按遥控器能力判断。
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
  // 解析和能力校验分开做：解析只关心文本是否合法，
  // acValidate() 再根据具体 remote 的能力判断能不能发送。
  if (validation != AcValidationError::Ok) {
    err(acValidationCode(validation), acValidationMessage(validation));
    return false;
  }
  return true;
}

// IRTEST 和空调控制分开处理，用来排查 GPIO、载波频率、
// 占空比以及红外接收器是否能看到基础信号。
// 它不依赖遥控器目录，所以即使空调协议还没匹配好，也能先验证硬件链路。
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

  // OK 响应只返回关键状态，避免把整条命令原样回显导致上位机解析复杂。
  Serial.printf("OK SENT remote=%s action=%s power=%u mode=%s temp=%.1f\n",
                ac.remote->id, acActionToString(ac.action),
                ac.state.power ? 1 : 0,
                acModeToString(ac.state.mode), ac.state.temp);
}

// 发送一条直接红外测试帧或连续载波，并返回实际使用的参数。
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

} // 命名空间

void serialProtocolPrintReady() {
  // READY 中带 protocol/baud/gpio，方便上位机日志直接确认固件版本和硬件接线。
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

  // 命令名统一转成大写，参数保持小写 key=value 文本，
  // 这样 Android USB 串口库实现起来更简单；参数大小写在各解析函数里统一处理。
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
