#pragma once

#include "ac_catalog.h"
#include "known_ac_runner.h"
#include "ac_store.h"
#include "serial_controller.h"

#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QStringList>
#include <QTimer>
#include <QTreeView>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSet>

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
    void renameAirConditioner();
    void deleteAirConditioner();
    void startKnownDevices();
    void stopKnownDevices();
    void openKnownDeviceControl(int row);
    void showCatalogPopup();
    void showIrTestMenu();
    void showLogWindow();
    void showSortMenu();
    void selectAllVisibleDevices();
    void clearSelectedDevices();
    void batchSetSelectedDevices();
    void stopBatchTask();
    void clearLog();
    void copyLog();
    void testIr38k();
    void testIr40k();

    // SerialController 信号入口。
    void appendSerialLine(const QString &line);
    void updateSerialStatus(const QString &status);
    void handleConnectionLost(const QString &reason);
    void sendHeartbeatPing();
    void sendNextBatchState();

private:
    // UI 构建和数据刷新。
    void buildUi();
    void populateCatalogTree();
    void reloadKnownDevices();
    void refreshKnownCards();
    void handleCatalogLine(const QString &line);
    QFrame *createKnownDeviceCard(int index);
    QVector<int> visibleDeviceIndexes() const;
    QVector<int> selectedDeviceIndexes() const;
    bool deviceMatchesSearch(int index) const;
    void updateSelectionControls();
    void rebuildKnownCardGrid();
    void updateConnectionActions();
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // 本地空调库和串口发送辅助函数。
    bool saveKnownDevices();
    void runKnownDevicesPower(bool targetPower);
    void renameDevice(int index);
    void duplicateDevice(int index);
    void deleteDeviceAt(int index);
    void showCardContextMenu(int index, const QPoint &globalPos);
    void sendIrTest(int freqHz);
    bool sendCommand(const QString &command);
    bool reconnectLastPort();
    void startHeartbeat();
    void stopHeartbeat();
    void markDisconnected(const QString &reason, bool showDialog);

    // 固定遥控器目录和用户已经匹配保存的空调列表。
    QVector<AcBrand> m_catalog;
    QVector<KnownAcDevice> m_knownDevices;
    QStringList m_pendingCatalogLines;
    bool m_receivingCatalog = false;
    int m_selectedDeviceIndex = -1;
    QSet<QString> m_selectedDeviceIds;
    QVector<int> m_batchDeviceIndexes;
    QVector<AcState> m_batchStates;
    QString m_searchText;
    int m_sortMode = 0;

    // 业务模块：JSON 存储、串口、批量开机定时器。
    AcStore m_store;
    SerialController m_serial;
    KnownAcRunner m_knownRunner;
    QTimer m_heartbeatTimer;
    QTimer m_batchStateTimer;
    bool m_waitingForPong = false;
    int m_missedPongs = 0;
    QString m_lastPortName;
    bool m_disconnectDialogVisible = false;

    QComboBox *m_portCombo = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_connectButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_catalogButton = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopAllButton = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_renameButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_irTestButton = nullptr;
    QPushButton *m_logButton = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QPushButton *m_sortButton = nullptr;
    QPushButton *m_batchSetButton = nullptr;
    QPushButton *m_stopTaskButton = nullptr;
    QLabel *m_selectionLabel = nullptr;
    QPushButton *m_selectAllButton = nullptr;
    QPushButton *m_clearSelectionButton = nullptr;
    QStandardItemModel *m_catalogModel = nullptr;
    QScrollArea *m_knownScrollArea = nullptr;
    QWidget *m_knownCardContainer = nullptr;
    QGridLayout *m_knownCardLayout = nullptr;
    QDialog *m_logDialog = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QPushButton *m_clearLogButton = nullptr;
    QPushButton *m_copyLogButton = nullptr;
};
