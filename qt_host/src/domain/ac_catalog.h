#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// 上位机侧保存的“空调目标状态”。
// 这些字段会被拼成固件串口协议里的 key=value，例如 mode=cool、temp=26。
// 字符串取值必须和 ESP32 固件 parseMode/parseFan/parseSwingV 等解析函数一致。
struct AcState {
  bool power = true;
  QString mode = "cool";
  int temp = 26;
  QString fan = "auto";
  QString swingv = "off";
  QString swingh = "off";
};

// 一个具体的遥控器候选项。
// id 是串口命令里的 remote=<id>，必须和固件 ac_catalog.cpp 中的遥控器 id 对齐。
// 支持能力用于限制 UI 控件范围，例如温度上下限、是否允许上下风。
struct AcRemote {
  QString id;
  QString brandId;
  QString name;
  int minTemp = 17;
  int maxTemp = 30;
  bool supportsFan = true;
  bool supportsSwingV = false;
  bool supportsSwingH = false;
};

// 品牌节点。一个品牌下面会挂多个遥控器候选，添加空调时按这个列表逐个测试。
struct AcBrand {
  QString id;
  QString name;
  QVector<AcRemote> remotes;
};

// 用户已经匹配并保存到本机 JSON 库里的空调。
// remoteId 指向某个 AcRemote；state 是上一次确认/发送成功后的状态快照。
struct KnownAcDevice {
  QString id;
  QString name;
  QString brandId;
  QString remoteId;
  AcState state;
};

// 第一版静态目录。后续可以改成启动时发送 CATALOG，从 ESP32 自动读取目录。
QVector<AcBrand> defaultAcCatalog();

// 解析 ESP32 返回的 CATALOG 文本行。解析失败或目录为空时返回空列表，
// 调用方继续使用 defaultAcCatalog() 作为 fallback。
QVector<AcBrand> catalogFromCatalogLines(const QStringList& lines);

// 按 remoteId 在品牌树里查找遥控器，用于显示名称、能力限制和控制界面。
const AcRemote* findRemote(const QVector<AcBrand>& catalog,
                           const QString& remoteId);

// 完整状态命令：用于添加空调时测试候选遥控器。
// 对 RN02S13 这类“功能键式”遥控器，完整状态可能会拆成多条红外码。
QString buildAcCommand(const QString& remoteId, const AcState& state);

// 单项动作命令：用于详情控制界面和一键开机。
// action=power/temp/mode/fan/swingv/swingh 能让固件只发被改动的那一个动作。
QString buildAcActionCommand(const QString& remoteId, const QString& action,
                             const AcState& state);

// UI 显示辅助函数，避免显示层到处手写同一套字符串拼接。
QString brandDisplayName(const AcBrand& brand);
QString defaultDeviceName(const AcRemote& remote);
