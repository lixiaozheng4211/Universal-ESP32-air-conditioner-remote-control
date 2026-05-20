#include "main_window.h"

#include "ac_control_dialog.h"
#include "ac_discovery_wizard.h"

#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSplitter>
#include <QStandardItem>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QString modeDisplayName(const QString &mode)
{
    if (mode == QStringLiteral("auto")) return QStringLiteral("自动");
    if (mode == QStringLiteral("cool")) return QStringLiteral("制冷");
    if (mode == QStringLiteral("heat")) return QStringLiteral("制热");
    if (mode == QStringLiteral("dry")) return QStringLiteral("除湿");
    if (mode == QStringLiteral("fan")) return QStringLiteral("送风");
    return mode;
}

QString fanDisplayName(const QString &fan)
{
    if (fan == QStringLiteral("auto")) return QStringLiteral("自动");
    if (fan == QStringLiteral("low")) return QStringLiteral("低风");
    if (fan == QStringLiteral("med")) return QStringLiteral("中风");
    if (fan == QStringLiteral("high")) return QStringLiteral("高风");
    if (fan == QStringLiteral("max")) return QStringLiteral("强风");
    return fan;
}

QString swingDisplayName(const QString &swing)
{
    return swing == QStringLiteral("auto") ? QStringLiteral("自动")
                                           : QStringLiteral("关闭");
}

}  // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_catalog(defaultAcCatalog())
{
    // 启动时先搭界面，再填目录和本地空调库，最后刷新串口列表。
    // 串口响应统一进入日志框，避免各个功能窗口自己处理 RX。
    buildUi();
    populateCatalogTree();
    reloadKnownDevices();
    refreshPorts();

    connect(&m_serial, &SerialController::lineReceived, this, &MainWindow::appendSerialLine);
    connect(&m_serial, &SerialController::statusChanged, this, &MainWindow::updateSerialStatus);
    connect(&m_serial, &SerialController::connectionLost, this, &MainWindow::handleConnectionLost);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &MainWindow::sendHeartbeatPing);
    m_heartbeatTimer.setInterval(3000);
}

void MainWindow::buildUi()
{
    // 主界面采用上中下结构：
    // 顶部串口连接区，中部操作按钮和空调列表，底部串口日志。
    setWindowTitle(QStringLiteral("Universal ESP32 AC Remote"));
    resize(980, 640);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    // 串口工具条：选择端口、刷新、连接/断开、显示连接状态。
    auto *serialLayout = new QHBoxLayout;
    m_portCombo = new QComboBox(central);
    m_portCombo->setMinimumWidth(180);
    m_refreshButton = new QPushButton(style()->standardIcon(QStyle::SP_BrowserReload), QString(), central);
    m_refreshButton->setToolTip(QStringLiteral("刷新串口"));
    m_connectButton = new QPushButton(QStringLiteral("连接"), central);
    m_statusLabel = new QLabel(QStringLiteral("未连接"), central);
    m_statusLabel->setMinimumWidth(160);

    serialLayout->addWidget(m_portCombo);
    serialLayout->addWidget(m_refreshButton);
    serialLayout->addWidget(m_connectButton);
    serialLayout->addWidget(m_statusLabel);
    serialLayout->addStretch();
    rootLayout->addLayout(serialLayout);

    // 主操作区。38K/40K 测试按钮用于排查红外硬件和载波频率问题。
    auto *actionLayout = new QHBoxLayout;
    m_startButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), QStringLiteral("开启空调"), central);
    m_stopAllButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop), QStringLiteral("关闭所有空调"), central);
    m_addButton = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogNewFolder), QStringLiteral("添加空调"), central);
    m_deleteButton = new QPushButton(style()->standardIcon(QStyle::SP_TrashIcon), QStringLiteral("删除空调"), central);
    m_ir38TestButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton), QStringLiteral("38K测试"), central);
    m_ir40TestButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton), QStringLiteral("40K测试"), central);
    m_startButton->setMinimumHeight(58);
    m_stopAllButton->setMinimumHeight(58);
    m_addButton->setMinimumHeight(58);
    m_deleteButton->setMinimumHeight(58);
    m_ir38TestButton->setMinimumHeight(58);
    m_ir40TestButton->setMinimumHeight(58);
    m_startButton->setIconSize(QSize(28, 28));
    m_stopAllButton->setIconSize(QSize(28, 28));
    m_addButton->setIconSize(QSize(28, 28));
    m_deleteButton->setIconSize(QSize(24, 24));
    m_ir38TestButton->setIconSize(QSize(24, 24));
    m_ir40TestButton->setIconSize(QSize(24, 24));
    m_stopAllButton->setToolTip(QStringLiteral("按间隔逐台发送关机命令"));
    m_deleteButton->setToolTip(QStringLiteral("从已保存空调库中删除当前空调"));
    m_ir38TestButton->setToolTip(QStringLiteral("发送 38kHz NEC 红外测试帧"));
    m_ir40TestButton->setToolTip(QStringLiteral("发送 40kHz NEC 红外测试帧"));
    actionLayout->addWidget(m_startButton);
    actionLayout->addWidget(m_stopAllButton);
    actionLayout->addWidget(m_addButton);
    actionLayout->addWidget(m_deleteButton);
    actionLayout->addWidget(m_ir38TestButton);
    actionLayout->addWidget(m_ir40TestButton);
    rootLayout->addLayout(actionLayout);

    // 左侧是静态遥控器目录，右侧是用户已经匹配成功的空调库。
    auto *splitter = new QSplitter(Qt::Horizontal, central);
    m_catalogTree = new QTreeView(splitter);
    m_catalogModel = new QStandardItemModel(this);
    m_catalogTree->setModel(m_catalogModel);
    m_catalogTree->setHeaderHidden(true);

    m_knownScrollArea = new QScrollArea(splitter);
    m_knownScrollArea->setWidgetResizable(true);
    m_knownScrollArea->setFrameShape(QFrame::NoFrame);
    m_knownCardContainer = new QWidget(m_knownScrollArea);
    m_knownCardLayout = new QGridLayout(m_knownCardContainer);
    m_knownCardLayout->setContentsMargins(8, 8, 8, 8);
    m_knownCardLayout->setSpacing(10);
    m_knownCardLayout->setAlignment(Qt::AlignTop);
    m_knownScrollArea->setWidget(m_knownCardContainer);

    splitter->addWidget(m_catalogTree);
    splitter->addWidget(m_knownScrollArea);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    rootLayout->addWidget(splitter, 1);

    // 串口日志同时记录 TX/RX，联调时可以直接看到 Qt 发了什么、固件回了什么。
    m_log = new QPlainTextEdit(central);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(400);
    m_log->setMinimumHeight(120);
    rootLayout->addWidget(m_log);

    setCentralWidget(central);

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startKnownDevices);
    connect(m_stopAllButton, &QPushButton::clicked, this, &MainWindow::stopKnownDevices);
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addAirConditioner);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteAirConditioner);
    connect(m_ir38TestButton, &QPushButton::clicked, this, &MainWindow::testIr38k);
    connect(m_ir40TestButton, &QPushButton::clicked, this, &MainWindow::testIr40k);
    updateConnectionActions();
}

void MainWindow::populateCatalogTree()
{
    // 品牌/遥控器目录只读展示；真正添加空调时会由 AcDiscoveryWizard 按品牌遍历候选。
    m_catalogModel->clear();
    for (const auto &brand : m_catalog) {
        auto *brandItem = new QStandardItem(brandDisplayName(brand));
        brandItem->setData(brand.id, Qt::UserRole);
        brandItem->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        for (const auto &remote : brand.remotes) {
            auto *remoteItem = new QStandardItem(remote.name);
            remoteItem->setData(remote.id, Qt::UserRole);
            remoteItem->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
            brandItem->appendRow(remoteItem);
        }
        m_catalogModel->appendRow(brandItem);
    }
    m_catalogTree->expandAll();
}

void MainWindow::reloadKnownDevices()
{
    // 从本机 JSON 加载用户保存的空调库。读取失败时仍刷新为空表，避免 UI 停在旧状态。
    QString error;
    m_knownDevices = m_store.load(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("读取失败"), error);
    }
    refreshKnownCards();
}

void MainWindow::refreshKnownCards()
{
    if (m_selectedDeviceIndex >= m_knownDevices.size()) {
        m_selectedDeviceIndex = m_knownDevices.isEmpty() ? -1 : m_knownDevices.size() - 1;
    }
    rebuildKnownCardGrid();
}

QFrame *MainWindow::createKnownDeviceCard(int index)
{
    const auto &device = m_knownDevices[index];
    const AcRemote *remote = findRemote(m_catalog, device.remoteId);
    const bool selected = index == m_selectedDeviceIndex;

    auto *card = new QFrame(m_knownCardContainer);
    card->setObjectName(QStringLiteral("knownAcCard"));
    card->setFrameShape(QFrame::StyledPanel);
    card->setCursor(Qt::PointingHandCursor);
    card->setMinimumSize(220, 156);
    card->setProperty("deviceIndex", index);
    card->setStyleSheet(QStringLiteral(
        "QFrame#knownAcCard {"
        "  border: 1px solid %1;"
        "  border-radius: 8px;"
        "  background: %2;"
        "}"
        "QLabel#title { font-size: 16px; font-weight: 600; }"
        "QLabel#stateOn { color: #0f7b3b; font-weight: 600; }"
        "QLabel#stateOff { color: #9a3412; font-weight: 600; }"
        "QLabel#meta { color: #59636e; }")
            .arg(selected ? QStringLiteral("#2563eb") : QStringLiteral("#d6dce3"),
                 selected ? QStringLiteral("#eef5ff") : QStringLiteral("#ffffff")));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(8);

    auto *topLayout = new QHBoxLayout;
    auto *title = new QLabel(device.name.isEmpty() ? device.remoteId : device.name, card);
    title->setObjectName(QStringLiteral("title"));
    title->setWordWrap(true);
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *state = new QLabel(device.state.power ? QStringLiteral("已开启") : QStringLiteral("已关闭"), card);
    state->setObjectName(device.state.power ? QStringLiteral("stateOn") : QStringLiteral("stateOff"));
    state->setAttribute(Qt::WA_TransparentForMouseEvents);
    topLayout->addWidget(title, 1);
    topLayout->addWidget(state);
    layout->addLayout(topLayout);

    auto *tempMode = new QLabel(QStringLiteral("%1°C  /  %2")
                                    .arg(device.state.temp)
                                    .arg(modeDisplayName(device.state.mode)),
                                card);
    tempMode->setObjectName(QStringLiteral("title"));
    tempMode->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(tempMode);

    auto *info = new QLabel(
        QStringLiteral("风速 %1    上下风 %2    左右风 %3")
            .arg(fanDisplayName(device.state.fan),
                 swingDisplayName(device.state.swingv),
                 swingDisplayName(device.state.swingh)),
        card);
    info->setObjectName(QStringLiteral("meta"));
    info->setWordWrap(true);
    info->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(info);

    auto *remoteInfo = new QLabel(
        QStringLiteral("%1 / %2")
            .arg(device.brandId, remote ? remote->name : device.remoteId),
        card);
    remoteInfo->setObjectName(QStringLiteral("meta"));
    remoteInfo->setWordWrap(true);
    remoteInfo->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(remoteInfo);
    layout->addStretch();

    card->installEventFilter(this);
    return card;
}

void MainWindow::setSelectedDeviceIndex(int index)
{
    if (index < 0 || index >= m_knownDevices.size()) {
        m_selectedDeviceIndex = -1;
    } else {
        m_selectedDeviceIndex = index;
    }
    rebuildKnownCardGrid();
}

void MainWindow::rebuildKnownCardGrid()
{
    while (QLayoutItem *item = m_knownCardLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    const int width = m_knownScrollArea ? m_knownScrollArea->viewport()->width() : 760;
    const int columns = qBound(1, width / 260, 3);
    for (int i = 0; i < m_knownDevices.size(); ++i) {
        m_knownCardLayout->addWidget(createKnownDeviceCard(i), i / columns, i % columns);
    }
    for (int column = 0; column < columns; ++column) {
        m_knownCardLayout->setColumnStretch(column, 1);
    }
}

void MainWindow::updateConnectionActions()
{
    const bool connected = m_serial.isOpen();
    m_startButton->setEnabled(connected);
    m_stopAllButton->setEnabled(connected);
    m_addButton->setEnabled(connected);
    m_ir38TestButton->setEnabled(connected);
    m_ir40TestButton->setEnabled(connected);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonDblClick) {
        if (auto *card = qobject_cast<QFrame *>(watched)) {
            const int index = card->property("deviceIndex").toInt();
            if (index >= 0 && index < m_knownDevices.size()) {
                m_selectedDeviceIndex = index;
                rebuildKnownCardGrid();
                if (event->type() == QEvent::MouseButtonDblClick) {
                    openKnownDeviceControl(index);
                }
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_knownCardLayout) {
        rebuildKnownCardGrid();
    }
}

void MainWindow::handleCatalogLine(const QString &line)
{
    if (line == QStringLiteral("OK CATALOG END")) {
        if (!m_receivingCatalog) {
            return;
        }

        const QVector<AcBrand> parsedCatalog = catalogFromCatalogLines(m_pendingCatalogLines);
        m_receivingCatalog = false;
        m_pendingCatalogLines.clear();
        if (!parsedCatalog.isEmpty()) {
            m_catalog = parsedCatalog;
            populateCatalogTree();
        }
        return;
    }

    if (line.startsWith(QStringLiteral("OK CATALOG "))) {
        m_receivingCatalog = true;
        m_pendingCatalogLines.clear();
        return;
    }

    if (m_receivingCatalog && line.startsWith(QStringLiteral("CAT "))) {
        m_pendingCatalogLines.push_back(line);
    }
}

void MainWindow::refreshPorts()
{
    // 保留当前选择：刷新串口列表后，如果原端口还在，就继续选中它。
    const QString current = m_portCombo->currentText();
    m_portCombo->clear();
    m_portCombo->addItems(m_serial.availablePorts());
    const int index = m_portCombo->findText(current);
    if (index >= 0) {
        m_portCombo->setCurrentIndex(index);
    }
}

void MainWindow::toggleConnection()
{
    // 连接成功后立即发送 PING，确认固件串口协议在线。
    if (m_serial.isOpen()) {
        markDisconnected(QString(), false);
        m_connectButton->setText(QStringLiteral("连接"));
        return;
    }

    QString error;
    if (!m_serial.open(m_portCombo->currentText(), &error)) {
        QMessageBox::warning(this, QStringLiteral("连接失败"), error);
        return;
    }
    m_lastPortName = m_portCombo->currentText();
    m_connectButton->setText(QStringLiteral("断开"));
    updateConnectionActions();
    startHeartbeat();
    sendCommand(QStringLiteral("CATALOG"));
}

void MainWindow::addAirConditioner()
{
    // 添加流程需要真实发送红外并让用户观察空调响应，所以必须先连接 ESP32。
    if (!m_serial.isOpen()) {
        QMessageBox::information(this, QStringLiteral("串口未连接"), QStringLiteral("请先连接 ESP32 串口。"));
        return;
    }

    AcDiscoveryWizard wizard(this, m_catalog);
    // 向导不直接持有串口；它通过这个回调把命令交给主窗口发送并记录日志。
    auto device = wizard.run([this](const QString &command) {
        return sendCommand(command);
    });
    if (!device) {
        return;
    }

    m_knownDevices.push_back(*device);

    if (!saveKnownDevices()) {
        return;
    }
    m_selectedDeviceIndex = m_knownDevices.size() - 1;
    refreshKnownCards();
    QMessageBox::information(this, QStringLiteral("已添加"), QStringLiteral("%1").arg(device->name));
}

void MainWindow::deleteAirConditioner()
{
    // 删除只影响电脑端 JSON，不会改 ESP32 内部状态。
    const int row = m_selectedDeviceIndex;
    if (row < 0 || row >= m_knownDevices.size()) {
        QMessageBox::information(this, QStringLiteral("未选择空调"), QStringLiteral("请先在空调库中选择一个空调。"));
        return;
    }

    const KnownAcDevice device = m_knownDevices[row];
    const auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除空调"),
        QStringLiteral("确定删除“%1”？").arg(device.name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    m_knownDevices.removeAt(row);
    if (!saveKnownDevices()) {
        return;
    }
    if (m_selectedDeviceIndex >= m_knownDevices.size()) {
        m_selectedDeviceIndex = m_knownDevices.size() - 1;
    }
    refreshKnownCards();
}

void MainWindow::startKnownDevices()
{
    runKnownDevicesPower(true);
}

void MainWindow::stopKnownDevices()
{
    runKnownDevicesPower(false);
}

void MainWindow::runKnownDevicesPower(bool targetPower)
{
    // 一键开关会复制当前设备列表交给 KnownAcRunner。
    // Runner 内部用 QTimer 间隔发送，主界面不会卡住。
    if (!m_serial.isOpen()) {
        QMessageBox::information(this, QStringLiteral("串口未连接"), QStringLiteral("请先连接 ESP32 串口。"));
        return;
    }
    if (m_knownDevices.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("空调库为空"), QStringLiteral("请先添加空调。"));
        return;
    }

    m_knownRunner.start(
        m_knownDevices,
        targetPower,
        [this](const QString &command) {
            return sendCommand(command);
        },
        [this](int index, const AcState &state) {
            if (index < 0 || index >= m_knownDevices.size()) {
                return;
            }
            m_knownDevices[index].state = state;
            saveKnownDevices();
            refreshKnownCards();
        });
}

void MainWindow::openKnownDeviceControl(int row)
{
    // 详情控制窗口只负责采集用户动作。真正发送 action 命令、保存状态和刷新表格都在这里完成。
    if (row < 0 || row >= m_knownDevices.size()) {
        return;
    }

    const AcRemote *remote = findRemote(m_catalog, m_knownDevices[row].remoteId);
    AcControlDialog dialog(this, m_knownDevices[row], remote, [this, row](const QString &action, const AcState &state) {
        if (row < 0 || row >= m_knownDevices.size()) {
            return false;
        }
        if (!m_serial.isOpen()) {
            QMessageBox::information(this, QStringLiteral("串口未连接"), QStringLiteral("请先连接 ESP32 串口。"));
            return false;
        }

        KnownAcDevice &device = m_knownDevices[row];
        // 详情页使用 action 命令，只发送用户实际改变的单项。
        // 这对 RN02S13 很重要，可以避免调温时连续发开机、模式、风速、温度多条红外。
        if (!sendCommand(buildAcActionCommand(device.remoteId, action, state))) {
            return false;
        }

        device.state = state;
        if (!saveKnownDevices()) {
            return false;
        }
        m_selectedDeviceIndex = row;
        refreshKnownCards();
        return true;
    });
    dialog.exec();
}

void MainWindow::testIr38k()
{
    sendIrTest(38000);
}

void MainWindow::testIr40k()
{
    sendIrTest(40000);
}

void MainWindow::sendIrTest(int freqHz)
{
    // IRTEST 不走空调协议，用固定 NEC 测试帧检查红外灯、三极管和接收头是否能看到载波。
    if (!m_serial.isOpen()) {
        QMessageBox::information(this, QStringLiteral("串口未连接"), QStringLiteral("请先连接 ESP32 串口。"));
        return;
    }
    sendCommand(QStringLiteral("IRTEST freq=%1 mode=nec count=4 duty=33").arg(freqHz));
}

bool MainWindow::saveKnownDevices()
{
    // 所有修改本地空调库的入口都走这里，统一处理保存失败提示。
    QString error;
    if (!m_store.save(m_knownDevices, &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
        return false;
    }
    return true;
}

void MainWindow::appendSerialLine(const QString &line)
{
    // RX 日志带时间戳，方便和 TX 对照分析固件响应延迟。
    m_log->appendPlainText(QStringLiteral("%1  RX  %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), line));
    if (line == QStringLiteral("OK PONG")) {
        m_waitingForPong = false;
        m_missedPongs = 0;
    }
    handleCatalogLine(line);
}

void MainWindow::updateSerialStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

bool MainWindow::sendCommand(const QString &command)
{
    // 所有串口发送都经过这里，保证失败提示和 TX 日志格式一致。
    QString error;
    if (!m_serial.sendLine(command, &error)) {
        if (!m_serial.isOpen()) {
            markDisconnected(error, true);
        } else {
            QMessageBox::warning(this, QStringLiteral("发送失败"), error);
        }
        return false;
    }
    m_log->appendPlainText(QStringLiteral("%1  TX  %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), command));
    return true;
}

void MainWindow::handleConnectionLost(const QString &reason)
{
    markDisconnected(reason, true);
}

void MainWindow::sendHeartbeatPing()
{
    if (!m_serial.isOpen()) {
        markDisconnected(QStringLiteral("串口未连接"), true);
        return;
    }

    if (m_waitingForPong) {
        ++m_missedPongs;
        if (m_missedPongs >= 2) {
            markDisconnected(QStringLiteral("PING 超时"), true);
            return;
        }
    }

    m_waitingForPong = true;
    sendCommand(QStringLiteral("PING"));
}

void MainWindow::startHeartbeat()
{
    m_waitingForPong = false;
    m_missedPongs = 0;
    m_heartbeatTimer.start();
    sendHeartbeatPing();
}

void MainWindow::stopHeartbeat()
{
    m_heartbeatTimer.stop();
    m_waitingForPong = false;
    m_missedPongs = 0;
}

void MainWindow::markDisconnected(const QString &reason, bool showDialog)
{
    const QString lastPort = m_lastPortName.isEmpty() ? m_serial.portName() : m_lastPortName;
    stopHeartbeat();
    m_knownRunner.stop();
    if (m_serial.isOpen()) {
        m_serial.close();
    }
    if (!lastPort.isEmpty()) {
        m_lastPortName = lastPort;
    }
    m_connectButton->setText(QStringLiteral("连接"));
    m_statusLabel->setText(QStringLiteral("未连接"));
    updateConnectionActions();

    if (!showDialog || m_disconnectDialogVisible) {
        return;
    }

    m_disconnectDialogVisible = true;
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("USB断开"));
    box.setText(QStringLiteral("USB已断开，请重新连接。"));
    if (!reason.isEmpty()) {
        box.setInformativeText(reason);
    }
    auto *reconnectButton =
        box.addButton(QStringLiteral("重新连接"), QMessageBox::AcceptRole);
    box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    box.exec();
    const bool reconnect = box.clickedButton() == reconnectButton;
    m_disconnectDialogVisible = false;

    if (reconnect) {
        reconnectLastPort();
    }
}

bool MainWindow::reconnectLastPort()
{
    refreshPorts();
    if (m_lastPortName.isEmpty() || m_portCombo->findText(m_lastPortName) < 0) {
        QMessageBox::information(this, QStringLiteral("未找到串口"),
                                 QStringLiteral("未找到原来的串口，请重新选择后连接。"));
        return false;
    }

    m_portCombo->setCurrentText(m_lastPortName);
    QString error;
    if (!m_serial.open(m_lastPortName, &error)) {
        QMessageBox::warning(this, QStringLiteral("重新连接失败"), error);
        return false;
    }

    m_connectButton->setText(QStringLiteral("断开"));
    updateConnectionActions();
    startHeartbeat();
    sendCommand(QStringLiteral("CATALOG"));
    return true;
}
