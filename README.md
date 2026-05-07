# Universal ESP32 空调串口遥控器

这个仓库实现了一个“串口空调遥控器”：上位设备通过 USB 串口给 ESP32 发送文本命令，ESP32 再选择对应空调遥控器协议并发射红外。

当前仓库包含两部分：

- `airconditioner_control_main/`：ESP32 固件主工程，负责串口协议解析、遥控器目录、IRremoteESP8266 红外发送。
- `qt_host/`：Qt6 Widgets 测试/参考上位机，用来在电脑上验证串口协议、添加空调和测试红外硬件。

Qt 上位机只是调试工具，不是最终平台限制。Android、Windows、Linux 或其它主控只要能打开串口并发送同样的文本命令，就可以控制 ESP32 发射空调红外。

Android 端开发请优先阅读：

```text
docs/android_serial_protocol.md
```

## 固件

红外发射引脚默认是 GPIO4，配置在：

```text
airconditioner_control_main/main/ac_config.h
```

串口参数：

```text
115200 8N1
每条命令以 \n 结尾
```

常用命令：

```text
PING
CATALOG
HELP
IRTEST freq=38000 mode=nec count=4 duty=33
IRTEST freq=40000 mode=nec count=4 duty=33
AC remote=midea_standard power=1 mode=cool temp=26 fan=auto swingv=off swingh=off eco=0
AC remote=midea_rn02s13 action=temp power=1 mode=cool temp=27 fan=auto swingv=off swingh=off eco=0
```

`CATALOG` 会输出当前固件内置的品牌和遥控器候选。当前版本暴露三种品牌：美的、格力、海尔。

`IRTEST` 用来排查硬件发射链路。默认发送 NEC 测试帧，普通红外接收模块更容易识别；如果要用示波器看连续载波，可以使用：

```text
IRTEST freq=38000 mode=carrier ms=300 count=2
```

注意：常见红外接收头是解调接收器，不适合直接判断连续载波；用 NEC 测试帧更可靠。

## Qt 测试上位机

Qt 工程位于：

```text
qt_host/
```

主界面包含串口连接区、开启空调、添加空调、删除空调、详情控制、38K 测试、40K 测试。

已发现并确认可用的空调保存在电脑端 `discovered_ac.json`。这份 JSON 只是 Qt 测试工具自己的本地库，Android 端可以用自己的数据库或配置文件保存 `remoteId` 和空调状态。

## 构建

ESP-IDF：

```powershell
. C:\esp\v5.5.3\esp-idf\export.ps1
idf.py -C airconditioner_control_main build
idf.py -C airconditioner_control_main -p COMx flash monitor
```

也可以先进入固件目录再构建：

```powershell
cd airconditioner_control_main
idf.py build
idf.py -p COMx flash monitor
```

仓库根目录提供了一个 ESP-IDF 兼容入口，但推荐始终使用 `-C airconditioner_control_main`，避免把构建产物放错位置。

Qt：

```powershell
qt-cmake.bat -S qt_host -B qt_host/build -G Ninja
cmake --build qt_host/build
```

如果 Qt 找不到串口模块，需要安装 Qt6 SerialPort 模块。
