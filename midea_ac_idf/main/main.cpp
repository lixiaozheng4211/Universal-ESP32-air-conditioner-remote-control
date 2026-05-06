#include "ir_Midea2.h"
#include <Arduino.h>
#include <IRsend.h>
#include <string.h>

// ==================== 调试宏 ====================
#define AC_DEBUG 1 // 设为 0 关闭调试输出

#if AC_DEBUG
#define AC_LOG(fmt, ...) Serial.printf("[AC] " fmt "\n", ##__VA_ARGS__)
#else
#define AC_LOG(fmt, ...)
#endif

// ==================== 空调状态 ====================
struct AcState {
  bool power;
  float temp;
  int mode;     // 0自动 1制冷 2制热 3抽湿 4送风
  int fanSpeed; // 0自动 1~5档
  bool eco;
  bool swingUD;
  bool swingLR;
  bool fzc; // 防直吹
};

AcState ac = {false, 26.0, 0, 0, false, false, false, false};

const uint8_t IR_LED = 4;
IRsendMeidi irsendmeidi(IR_LED);

// ==================== 辅助函数 ====================
const char *modeStr(int m) {
  switch (m) {
  case 0:
    return "自动";
  case 1:
    return "制冷";
  case 2:
    return "制热";
  case 3:
    return "抽湿";
  case 4:
    return "送风";
  default:
    return "未知";
  }
}

const char *fanStr(int f) {
  switch (f) {
  case 0:
    return "自动";
  case 1:
    return "20%";
  case 2:
    return "40%";
  case 3:
    return "60%";
  case 4:
    return "80%";
  case 5:
    return "100%";
  default:
    return "未知";
  }
}

const char *onOff(bool v) { return v ? "开" : "关"; }

// 打印状态面板
void printStatus() {
  Serial.println();
  Serial.println("╔══════════════════════════════════╗");
  Serial.println("║        美的空调控制面板          ║");
  Serial.println("╠══════════════════════════════════╣");
  Serial.printf(" ║  电源状态 : %-20s  ║\n", onOff(ac.power));
  Serial.printf(" ║  当前温度 : %-18.1f°C ║\n", ac.temp);
  Serial.printf(" ║  运行模式 : %-20s   ║\n", modeStr(ac.mode));
  Serial.printf(" ║  风速档位 : %-20s   ║\n", fanStr(ac.fanSpeed));
  Serial.printf(" ║  ECO模式  : %-20s  ║\n", onOff(ac.eco));
  Serial.printf(" ║  上下扫风 : %-20s  ║\n", onOff(ac.swingUD));
  Serial.printf(" ║  左右扫风 : %-20s  ║\n", onOff(ac.swingLR));
  Serial.printf(" ║  防直吹   : %-20s  ║\n", onOff(ac.fzc));
  Serial.println("╚══════════════════════════════════╝");
  Serial.println();
  Serial.print(">> 请输入命令 (输入 help 查看帮助): ");
}

// 打印帮助
void printHelp() {
  Serial.println();
  Serial.println("┌──────────────────────────────────┐");
  Serial.println("│           可用命令列表            │");
  Serial.println("├──────────────────────────────────┤");
  Serial.println("│  on         - 开机               │");
  Serial.println("│  off        - 关机               │");
  Serial.println("│  temp <值>  - 设置温度 (17~30)   │");
  Serial.println("│  mode <值>  - 设置模式           │");
  Serial.println("│    0=自动 1=制冷 2=制热          │");
  Serial.println("│    3=抽湿 4=送风                 │");
  Serial.println("│  fan <值>   - 设置风速            │");
  Serial.println("│    0=自动 1=20% 2=40% 3=60%     │");
  Serial.println("│    4=80% 5=100%                  │");
  Serial.println("│  eco <0/1>  - ECO开关            │");
  Serial.println("│  swud <0/1> - 上下扫风开关       │");
  Serial.println("│  swlr <0/1> - 左右扫风开关       │");
  Serial.println("│  fzc <0/1>  - 防直吹开关         │");
  Serial.println("│  status     - 显示当前状态       │");
  Serial.println("│  help       - 显示帮助           │");
  Serial.println("└──────────────────────────────────┘");
}

// ==================== 命令处理 ====================
void processCommand(const char *cmd) {
  char command[32] = {0};
  float value = 0;
  int parsed = sscanf(cmd, "%31s %f", command, &value);

  if (parsed < 1)
    return;

  // -- on --
  if (strcmp(command, "on") == 0) {
    AC_LOG(">>> 开机，温度 %.1f°C", ac.temp);
    irsendmeidi.setPowers(1);
    ac.power = true;
    AC_LOG("<<< 开机完成");
  }
  // -- off --
  else if (strcmp(command, "off") == 0) {
    AC_LOG(">>> 关机");
    irsendmeidi.setPowers(0);
    ac.power = false;
    AC_LOG("<<< 关机完成");
  }
  // -- temp --
  else if (strcmp(command, "temp") == 0 && parsed == 2) {
    if (value < 17.0 || value > 30.0) {
      Serial.println("[错误] 温度范围: 17.0 ~ 30.0");
    } else {
      AC_LOG(">>> 设置温度: %.1f°C -> %.1f°C", ac.temp, value);
      ac.temp = value;
      irsendmeidi.setTemps(ac.temp);
      AC_LOG("<<< 温度设置完成");
    }
  }
  // -- mode --
  else if (strcmp(command, "mode") == 0 && parsed == 2) {
    int m = (int)value;
    if (m < 0 || m > 4) {
      Serial.println("[错误] 模式范围: 0~4");
    } else {
      AC_LOG(">>> 设置模式: %s -> %s", modeStr(ac.mode), modeStr(m));
      ac.mode = m;
      irsendmeidi.setModes(ac.mode);
      AC_LOG("<<< 模式设置完成");
    }
  }
  // -- fan --
  else if (strcmp(command, "fan") == 0 && parsed == 2) {
    int f = (int)value;
    if (f < 0 || f > 5) {
      Serial.println("[错误] 风速范围: 0~5");
    } else {
      AC_LOG(">>> 设置风速: %s -> %s", fanStr(ac.fanSpeed), fanStr(f));
      ac.fanSpeed = f;
      irsendmeidi.setFanSpeeds(ac.fanSpeed);
      AC_LOG("<<< 风速设置完成");
    }
  }
  // -- eco --
  else if (strcmp(command, "eco") == 0 && parsed == 2) {
    bool v = (int)value;
    AC_LOG(">>> ECO: %s -> %s", onOff(ac.eco), onOff(v));
    ac.eco = v;
    irsendmeidi.setEco(ac.eco);
    AC_LOG("<<< ECO设置完成");
  }
  // -- swud (上下扫风) --
  else if (strcmp(command, "swud") == 0 && parsed == 2) {
    bool v = (int)value;
    AC_LOG(">>> 上下扫风: %s -> %s", onOff(ac.swingUD), onOff(v));
    ac.swingUD = v;
    irsendmeidi.setSwingUD(ac.swingUD);
    AC_LOG("<<< 上下扫风设置完成");
  }
  // -- swlr (左右扫风) --
  else if (strcmp(command, "swlr") == 0 && parsed == 2) {
    bool v = (int)value;
    AC_LOG(">>> 左右扫风: %s -> %s", onOff(ac.swingLR), onOff(v));
    ac.swingLR = v;
    irsendmeidi.setSwingLR(ac.swingLR);
    AC_LOG("<<< 左右扫风设置完成");
  }
  // -- fzc (防直吹) --
  else if (strcmp(command, "fzc") == 0 && parsed == 2) {
    bool v = (int)value;
    AC_LOG(">>> 防直吹: %s -> %s", onOff(ac.fzc), onOff(v));
    ac.fzc = v;
    irsendmeidi.setFZC(ac.fzc);
    AC_LOG("<<< 防直吹设置完成");
  }
  // -- status --
  else if (strcmp(command, "status") == 0) {
    printStatus();
    return; // status 自带提示符，不再重复
  }
  // -- help --
  else if (strcmp(command, "help") == 0) {
    printHelp();
  }
  // -- 未知命令 --
  else {
    Serial.printf("[错误] 未知命令: %s (输入 help 查看帮助)\n", command);
  }

  // 操作后刷新状态
  printStatus();
}

// ==================== 主程序 ====================
void setup() {
  Serial.begin(115200);
  delay(1000); // 等待串口稳定

  irsendmeidi.begin_2();
  irsendmeidi.setZBPL(40);
  irsendmeidi.setCodeTime(500, 1600, 550, 4400, 4400, 5220);

  AC_LOG("系统初始化完成");
  AC_LOG("IR LED: GPIO %d, 载波: 40kHz", IR_LED);

  printHelp();
  printStatus();
}

void loop() {
  // 读取串口命令
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      AC_LOG("收到命令: \"%s\"", input.c_str());
      processCommand(input.c_str());
    }
  }

  delay(100); // 防止看门狗超时 + 降低 CPU 占用
}