# Android 串口接入协议说明

本文档面向 Android 端软件开发人员。Android App 只需要通过 USB 串口向 ESP32-S3 发送文本命令，就可以控制 ESP32-S3 发射空调红外编码。

## 1. 串口连接

V1.1 固件目标芯片为 ESP32-S3，默认仍使用 UART0 / USB-UART 桥接方式，不启用 USB CDC 作为主控制串口。串口参数固定为：

```text
baudrate: 115200
data bits: 8
parity: none
stop bits: 1
flow control: none
line ending: \n
encoding: UTF-8 或 ASCII
```

Android 端推荐流程：

1. 通过 USB Host 串口库打开 ESP32 串口。
2. 发送 `PING\n`。
3. 收到 `OK PONG` 后认为设备在线。
4. 发送 `CATALOG\n` 读取当前固件支持的品牌和遥控器候选。
5. App 保存已确认可用空调的 `remoteId` 和状态。
6. 后续控制时发送 `AC ...\n` 命令。

每条命令一行，ESP32 每条响应也是一行。Android 端读取串口时要按 `\n` 拆行，不能假设一次 read 就一定是一整行。

## 2. 基础命令

### PING

用于检测串口协议是否在线。

```text
PING
```

成功响应：

```text
OK PONG
```

### HELP

查看固件支持的命令。

```text
HELP
```

示例响应：

```text
OK COMMANDS PING CATALOG AC(action=state/power/temp/mode/fan/swingv/swingh) IRTEST
```

### CATALOG

读取 ESP32 固件内置的品牌和遥控器候选。

```text
CATALOG
```

响应格式示例：

```text
OK CATALOG remotes=88
CAT BRAND id=midea name="Midea" first_child=16 next_sibling=1
CAT REMOTE id=midea_standard brand=midea name="Midea standard" temp=17-30 fan=1 swingv=1 swingh=0 driver="IRac" next_sibling=17
CAT REMOTE id=midea_rn02s13 brand=midea name="Midea RN02S13" temp=17-30 fan=1 swingv=1 swingh=1 driver="Midea RN02S13" next_sibling=-1
OK CATALOG END
```

Android 端至少需要保存 `CAT REMOTE` 行中的：

- `id`：后续 `AC remote=<id>` 使用。
- `brand`：用于 UI 分组。
- `name`：用于 UI 展示。
- `temp`：温度范围。
- `fan` / `swingv` / `swingh`：能力开关。

当前版本只暴露 IRremoteESP8266/IRac 能直接发送的品牌和协议。内置品牌包括：美的、格力、海尔、TCL、科龙、大金、日立、松下、三菱电机、三菱重工、东芝、夏普、三星、LG、开利、奥克斯、Airton、Airwell、Amcor、Argo、博世、Coolix、Corona、德龙、Ecoclim、Eurom、富士通、Goodweather、Kelvinator、Mirage、Neoclima、Rhoss、三洋、Teknopoint、Technibel、TECO、Trotec、Truma、Vestel、Voltas、惠而浦、Transcold。小米、海信、长虹、约克等没有接入 `IRac::sendAc()` 统一发送分支的品牌先不加入本版目录。

## 3. 空调控制命令

空调控制命令统一使用 `AC`：

```text
AC remote=<id> action=<action> power=<0|1> mode=<auto|cool|heat|dry|fan> temp=<17..30> fan=<auto|low|med|high|max> swingv=<off|auto> swingh=<off|auto>
```

字段说明：

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `remote` | 是 | 遥控器 ID，来自 `CATALOG` 的 `CAT REMOTE id=` |
| `action` | 否 | 动作类型；不写时默认为 `state` |
| `power` | 是 | `1` 开机，`0` 关机 |
| `mode` | 是 | `auto` 自动，`cool` 制冷，`heat` 制热，`dry` 除湿，`fan` 送风 |
| `temp` | 是 | 温度，必须在该遥控器支持范围内 |
| `fan` | 是 | 风速，建议默认 `auto` |
| `swingv` | 是 | 上下风，`off` 或 `auto` |
| `swingh` | 是 | 左右风，`off` 或 `auto` |

兼容说明：旧客户端如果仍发送 `eco=0` 或 `eco=1`，当前固件会解析并忽略该字段；新客户端不要再发送 `eco`。

### action 取值

| action | 用途 |
| --- | --- |
| `state` | 完整状态同步；适合添加空调时测试候选遥控器 |
| `power` | 只发送开/关机动作 |
| `temp` | 只发送温度调整动作 |
| `mode` | 只发送模式调整动作 |
| `fan` | 只发送风速调整动作 |
| `swingv` | 只发送上下风动作 |
| `swingh` | 只发送左右风动作 |

建议 Android 端日常控制优先使用单项 `action`。例如只调温时发送 `action=temp`，不要每次都发送完整状态。这样可以避免部分遥控器一次控制触发多条红外码。

## 4. 常用示例

检测设备：

```text
PING
```

开机：

```text
AC remote=midea_rn02s13 action=power power=1 mode=cool temp=26 fan=auto swingv=off swingh=off
```

关机：

```text
AC remote=midea_rn02s13 action=power power=0 mode=cool temp=26 fan=auto swingv=off swingh=off
```

调温到 27 摄氏度：

```text
AC remote=midea_rn02s13 action=temp power=1 mode=cool temp=27 fan=auto swingv=off swingh=off
```

切换制热：

```text
AC remote=midea_rn02s13 action=mode power=1 mode=heat temp=26 fan=auto swingv=off swingh=off
```

调整风速：

```text
AC remote=midea_rn02s13 action=fan power=1 mode=cool temp=26 fan=high swingv=off swingh=off
```

打开上下风：

```text
AC remote=midea_rn02s13 action=swingv power=1 mode=cool temp=26 fan=auto swingv=auto swingh=off
```

打开左右风：

```text
AC remote=midea_rn02s13 action=swingh power=1 mode=cool temp=26 fan=auto swingv=off swingh=auto
```

添加空调时测试完整状态：

```text
AC remote=midea_standard power=1 mode=cool temp=26 fan=auto swingv=off swingh=off
```

## 5. 响应与错误处理

成功响应统一以 `OK` 开头：

```text
OK SENT remote=midea_rn02s13 action=temp power=1 mode=cool temp=27.0
```

错误响应统一以 `ERR` 开头：

```text
ERR UNKNOWN_REMOTE midea_xxx
ERR BAD_ACTION use state/power/temp/mode/fan/swingv/swingh
ERR BAD_TEMP temperature out of remote range
ERR SEND_FAILED midea_rn02s13
```

Android 端建议：

- 收到 `OK SENT` 后再更新 App 内保存的空调状态。
- 收到 `ERR` 时不要更新本地状态，并把错误信息展示或写入日志。
- 批量控制多个空调时，每条 `AC` 命令之间至少间隔 `1500ms`。

## 6. 红外硬件测试

`IRTEST` 只用于硬件调试，不属于日常空调控制。

发送 38kHz NEC 测试帧：

```text
IRTEST freq=38000 mode=nec count=4 duty=33
```

发送 40kHz NEC 测试帧：

```text
IRTEST freq=40000 mode=nec count=4 duty=33
```

如果使用示波器观察连续载波：

```text
IRTEST freq=38000 mode=carrier ms=300 count=2
```

常见红外接收头是解调接收器，不能稳定检测连续载波。排查普通接收模块时优先使用 `mode=nec`。
