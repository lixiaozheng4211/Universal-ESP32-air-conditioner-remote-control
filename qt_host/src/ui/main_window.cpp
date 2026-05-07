#include "main_window.h"

#include "ac_control_dialog.h"
#include "ac_discovery_wizard.h"

#include <QApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
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
    m_addButton = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogNewFolder), QStringLiteral("添加空调"), central);
    m_deleteButton = new QPushButton(style()->standardIcon(QStyle::SP_TrashIcon), QStringLiteral("删除空调"), central);
    m_ir38TestButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton), QStringLiteral("38K测试"), central);
    m_ir40TestButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton), QStringLiteral("40K测试"), central);
    m_startButton->setMinimumHeight(58);
    m_addButton->setMinimumHeight(58);
    m_deleteButton->setMinimumHeight(58);
    m_ir38TestButton->setMinimumHeight(58);
    m_ir40TestButton->setMinimumHeight(58);
    m_startButton->setIconSize(QSize(28, 28));
    m_addButton->setIconSize(QSize(28, 28));
    m_deleteButton->setIconSize(QSize(24, 24));
    m_ir38TestButton->setIconSize(QSize(24, 24));
    m_ir40TestButton->setIconSize(QSize(24, 24));
    m_deleteButton->setToolTip(QStringLiteral("从已保存空调库中删除当前空调"));
    m_ir38TestButton->setToolTip(QStringLiteral("发送 38kHz NEC 红外测试帧"));
    m_ir40TestButton->setToolTip(QStringLiteral("发送 40kHz NEC 红外测试帧"));
    actionLayout->addWidget(m_startButton);
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

    m_knownTable = new QTableWidget(splitter);
    m_knownTable->setColumnCount(9);
    m_knownTable->setHorizontalHeaderLabels({
        QStringLiteral("名称"),
        QStringLiteral("品牌"),
        QStringLiteral("遥控器"),
        QStringLiteral("状态"),
        QStringLiteral("温度"),
        QStringLiteral("模式"),
        QStringLiteral("风速"),
        QStringLiteral("上下风"),
        QStringLiteral("左右风"),
    });
    m_knownTable->horizontalHeader()->setStretchLastSection(true);
    m_knownTable->verticalHeader()->setVisible(false);
    m_knownTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_knownTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    splitter->addWidget(m_catalogTree);
    splitter->addWidget(m_knownTable);
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
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addAirConditioner);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteAirConditioner);
    connect(m_knownTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::openKnownDeviceControl);
    connect(m_ir38TestButton, &QPushButton::clicked, this, &MainWindow::testIr38k);
    connect(m_ir40TestButton, &QPushButton::clicked, this, &MainWindow::testIr40k);
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
    refreshKnownTable();
}

void MainWindow::refreshKnownTable()
{
    // 表格只展示关键状态。单击只负责选中，双击才进入详情控制窗口。
    m_knownTable->setRowCount(m_knownDevices.size());
    for (int row = 0; row < m_knownDevices.size(); ++row) {
        const auto &device = m_knownDevices[row];
        const AcRemote *remote = findRemote(m_catalog, device.remoteId);
        m_knownTable->setItem(row, 0, new QTableWidgetItem(device.name));
        m_knownTable->setItem(row, 1, new QTableWidgetItem(device.brandId));
        m_knownTable->setItem(row, 2, new QTableWidgetItem(remote ? remote->name : device.remoteId));
        m_knownTable->setItem(row, 3, new QTableWidgetItem(device.state.power ? QStringLiteral("开") : QStringLiteral("关")));
        m_knownTable->setItem(row, 4, new QTableWidgetItem(QString::number(device.state.temp)));
        m_knownTable->setItem(row, 5, new QTableWidgetItem(modeDisplayName(device.state.mode)));
        m_knownTable->setItem(row, 6, new QTableWidgetItem(fanDisplayName(device.state.fan)));
        m_knownTable->setItem(row, 7, new QTableWidgetItem(swingDisplayName(device.state.swingv)));
        m_knownTable->setItem(row, 8, new QTableWidgetItem(swingDisplayName(device.state.swingh)));
    }
    m_knownTable->resizeColumnsToContents();
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
        m_serial.close();
        m_connectButton->setText(QStringLiteral("连接"));
        return;
    }

    QString error;
    if (!m_serial.open(m_portCombo->currentText(), &error)) {
        QMessageBox::warning(this, QStringLiteral("连接失败"), error);
        return;
    }
    m_connectButton->setText(QStringLiteral("断开"));
    sendCommand(QStringLiteral("PING"));
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
    refreshKnownTable();
    QMessageBox::information(this, QStringLiteral("已添加"), QStringLiteral("%1").arg(device->name));
}

void MainWindow::deleteAirConditioner()
{
    // 删除只影响电脑端 JSON，不会改 ESP32 内部状态。
    const int row = m_knownTable->currentRow();
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
    refreshKnownTable();
}

void MainWindow::startKnownDevices()
{
    // 一键开启会复制当前设备列表交给 KnownAcRunner。
    // Runner 内部用 QTimer 间隔发送，主界面不会卡住。
    if (!m_serial.isOpen()) {
        QMessageBox::information(this, QStringLiteral("串口未连接"), QStringLiteral("请先连接 ESP32 串口。"));
        return;
    }
    if (m_knownDevices.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("空调库为空"), QStringLiteral("请先添加空调。"));
        return;
    }

    m_knownRunner.start(m_knownDevices, [this](const QString &command) {
        return sendCommand(command);
    });
}

void MainWindow::openKnownDeviceControl(int row, int column)
{
    Q_UNUSED(column);

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
        refreshKnownTable();
        if (row < m_knownTable->rowCount()) {
            m_knownTable->selectRow(row);
        }
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
        QMessageBox::warning(this, QStringLiteral("发送失败"), error);
        return false;
    }
    m_log->appendPlainText(QStringLiteral("%1  TX  %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), command));
    return true;
}
