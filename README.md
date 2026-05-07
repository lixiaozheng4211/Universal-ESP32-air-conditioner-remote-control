# Universal ESP32 空调串口遥控器（serial-only）

这个分支只保留 ESP32 串口遥控器固件，不包含 Qt 测试上位机。Android、Windows、Linux 或其它主控只要能打开 USB 串口并发送文本命令，就可以控制 ESP32 发射空调红外。

当前版本只内置三种品牌候选遥控器：

- 美的
- 格力
- 海尔

Android 端开发请阅读：

```text
docs/android_serial_protocol.md
```

## 固件工程

ESP-IDF 工程目录：

```text
airconditioner_control_main/
```

红外发射引脚默认是 GPIO4，配置在：

```text
airconditioner_control_main/main/ac_config.h
```

当前 `sdkconfig` 保持已调试通过的 `esp32` target，不在本分支切换到 `esp32s3`。

## 串口协议

串口参数：

```text
115200 8N1
每条命令以 \n 结尾
```

基础命令：

```text
PING
HELP
CATALOG
IRTEST freq=38000 mode=nec count=4 duty=33
AC remote=midea_rn02s13 action=power power=1 mode=cool temp=26 fan=auto swingv=off swingh=off eco=0
```

`CATALOG` 会输出固件内置的品牌和遥控器候选。Android 端应保存已确认可用的 `remoteId`，后续使用 `AC remote=<remoteId> ...` 控制空调。

日常控制建议使用单项动作：

```text
action=power
action=temp
action=mode
action=swingv
```

添加/匹配空调时可以使用不带 `action` 的完整状态命令。

## 构建与烧录

```powershell
. C:\esp\v5.5.3\esp-idf\export.ps1
idf.py -C airconditioner_control_main build
idf.py -C airconditioner_control_main -p COMx flash monitor
```

也可以进入固件目录：

```powershell
cd airconditioner_control_main
idf.py build
idf.py -p COMx flash monitor
```

仓库根目录保留了 ESP-IDF 兼容 `CMakeLists.txt`，但推荐使用 `-C airconditioner_control_main` 明确指定固件工程目录。

## 硬件测试

`IRTEST` 用于排查红外发射硬件：

```text
IRTEST freq=38000 mode=nec count=4 duty=33
IRTEST freq=40000 mode=nec count=4 duty=33
```

如果使用示波器观察连续载波：

```text
IRTEST freq=38000 mode=carrier ms=300 count=2
```

常见红外接收头是解调接收器，不适合直接判断连续载波；用 NEC 测试帧更可靠。
