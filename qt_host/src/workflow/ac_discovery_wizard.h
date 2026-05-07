#pragma once

#include "ac_catalog.h"

#include <QWidget>

#include <functional>
#include <optional>

// 添加空调向导。
// 流程是：选择品牌 -> 依次测试该品牌下的遥控器候选 -> 用户确认开机响应 ->
// 再确认温度可调 -> 保存为 KnownAcDevice。
// 这个类不保存文件，也不直接操作串口；串口发送通过 SendCommand 回调交给 MainWindow。
class AcDiscoveryWizard {
public:
    // 返回 true 表示命令成功写入串口。这里不等待空调物理响应，物理响应由用户确认。
    using SendCommand = std::function<bool(const QString &command)>;

    AcDiscoveryWizard(QWidget *parent, const QVector<AcBrand> &catalog);

    std::optional<KnownAcDevice> run(const SendCommand &sendCommand);

private:
    // 弹出品牌选择框，返回用户选择的品牌节点。
    const AcBrand *selectBrand() const;

    // 两个确认框分别对应“是否开机响应”和“温度是否可调”。
    bool confirmPowerResponse(const AcRemote &remote) const;
    bool confirmTempResponse(const AcRemote &remote) const;

    // 匹配成功后让用户命名，默认名称来自遥控器型号。
    QString askDeviceName(const AcRemote &remote, bool *accepted) const;

    QWidget *m_parent = nullptr;

    // 引用 MainWindow 持有的目录，避免复制整棵品牌/遥控器列表。
    const QVector<AcBrand> &m_catalog;
};
