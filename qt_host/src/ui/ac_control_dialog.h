#pragma once

#include "ac_catalog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

#include <functional>

// 已保存空调的详情控制窗口。
// 这个窗口不直接碰串口，只把“用户想做的动作”和“动作后的状态”回调给 MainWindow。
// 这样 UI 只负责收集输入，串口发送、保存 JSON、刷新表格仍由主窗口统一处理。
class AcControlDialog : public QDialog {
    Q_OBJECT

public:
    // action 对应固件协议里的 action=power/temp/mode/fan/swingv/swingh。
    // state 是执行这个 action 后希望保存的完整状态快照。
    using SendAction = std::function<bool(const QString &action, const AcState &state)>;

    AcControlDialog(QWidget *parent, const KnownAcDevice &device,
                    const AcRemote *remote, SendAction sendAction);

    AcState state() const;

private:
    // 创建控件和信号连接。
    void buildUi();

    // 把 m_state 同步回界面控件。发送成功或切换设备状态后调用。
    void syncUiFromState();

    // 从界面读取温度、模式、风速、上下风、左右风，形成一个候选状态。
    AcState uiState() const;

    // “发送控制”按钮入口：比较 m_state 和 uiState，只发送实际变化的项。
    bool sendUiState();

    // 开机/关机按钮入口：只发送 power，不顺手保存界面上还没点击发送的温度/模式。
    void setPowerAndSend(bool power);

    // m_device 是打开窗口时的设备信息；m_state 是窗口内当前确认成功的状态。
    KnownAcDevice m_device;
    const AcRemote *m_remote = nullptr;
    SendAction m_sendAction;
    AcState m_state;

    QLabel *m_titleLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_powerOnButton = nullptr;
    QPushButton *m_powerOffButton = nullptr;
    QSpinBox *m_tempSpin = nullptr;
    QComboBox *m_modeCombo = nullptr;
    QComboBox *m_fanCombo = nullptr;
    QCheckBox *m_swingVCheck = nullptr;
    QCheckBox *m_swingHCheck = nullptr;
    QPushButton *m_sendButton = nullptr;
};
