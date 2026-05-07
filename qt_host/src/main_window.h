#pragma once

#include "ac_catalog.h"
#include "known_ac_runner.h"
#include "ac_store.h"
#include "serial_controller.h"

#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QTreeView>
#include <QComboBox>

// 主窗口负责把所有模块串起来：
// 1. 串口选择/连接；
// 2. 空调目录展示；
// 3. 已保存空调库的增删改；
// 4. 一键开机、详情控制、红外测试；
// 5. 串口 TX/RX 日志展示。
// 具体子流程尽量拆给 AcDiscoveryWizard、AcControlDialog、KnownAcRunner 等类。
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // 串口和主操作按钮。
    void refreshPorts();
    void toggleConnection();
    void addAirConditioner();
    void deleteAirConditioner();
    void startKnownDevices();
    void openKnownDeviceControl(int row, int column);
    void testIr38k();
    void testIr40k();

    // SerialController 信号入口。
    void appendSerialLine(const QString &line);
    void updateSerialStatus(const QString &status);

private:
    // UI 构建和数据刷新。
    void buildUi();
    void populateCatalogTree();
    void reloadKnownDevices();
    void refreshKnownTable();

    // 本地空调库和串口发送辅助函数。
    bool saveKnownDevices();
    void sendIrTest(int freqHz);
    bool sendCommand(const QString &command);

    // 固定遥控器目录和用户已经匹配保存的空调列表。
    QVector<AcBrand> m_catalog;
    QVector<KnownAcDevice> m_knownDevices;

    // 业务模块：JSON 存储、串口、批量开机定时器。
    AcStore m_store;
    SerialController m_serial;
    KnownAcRunner m_knownRunner;

    QComboBox *m_portCombo = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_connectButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_ir38TestButton = nullptr;
    QPushButton *m_ir40TestButton = nullptr;
    QTreeView *m_catalogTree = nullptr;
    QStandardItemModel *m_catalogModel = nullptr;
    QTableWidget *m_knownTable = nullptr;
    QPlainTextEdit *m_log = nullptr;
};
