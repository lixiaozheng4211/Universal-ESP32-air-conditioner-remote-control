#pragma once

#include <Arduino.h>

// 按行解析的串口协议，可供 Qt、Android 或手动串口终端使用。
// 这里故意不用 JSON/二进制帧：文本 key=value 对 Android 串口库、
// 串口调试助手和人工排查都更友好，ESP32 侧解析成本也低。
//
// 除 CATALOG 会在 OK 标记之间输出多行 CAT 外，其它命令都会同步返回一条 OK 或 ERR。
// 这个规则让上位机不用猜一条命令什么时候结束，日志也更容易读。
void serialProtocolPrintReady();
void serialProtocolProcessLine(String line);
