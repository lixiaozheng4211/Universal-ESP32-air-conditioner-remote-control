#include "main_window.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_catalog(defaultAcCatalog())
{
    buildUi();
    populateCatalogTree();
    reloadKnownDevices();
    refreshPorts();

    connect(&m_serial, &SerialController::lineReceived, this, &MainWindow::appendSerialLine);
    connect(&m_serial, &SerialController::statusChanged, this, &MainWindow::updateSerialStatus);
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Universal ESP32 AC Remote"));
    resize(980, 640);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

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

    auto *actionLayout = new QHBoxLayout;
    m_startButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), QStringLiteral("开启空调"), central);
    m_addButton = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogNewFolder), QStringLiteral("添加空调"), central);
    m_startButton->setMinimumHeight(58);
    m_addButton->setMinimumHeight(58);
    m_startButton->setIconSize(QSize(28, 28));
    m_addButton->setIconSize(QSize(28, 28));
    actionLayout->addWidget(m_startButton);
    actionLayout->addWidget(m_addButton);
    rootLayout->addLayout(actionLayout);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    m_catalogTree = new QTreeView(splitter);
    m_catalogModel = new QStandardItemModel(this);
    m_catalogTree->setModel(m_catalogModel);
    m_catalogTree->setHeaderHidden(true);

    m_knownTable = new QTableWidget(splitter);
    m_knownTable->setColumnCount(4);
    m_knownTable->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("品牌"), QStringLiteral("遥控器"), QStringLiteral("温度")});
    m_knownTable->horizontalHeader()->setStretchLastSection(true);
    m_knownTable->verticalHeader()->setVisible(false);
    m_knownTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_knownTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    splitter->addWidget(m_catalogTree);
    splitter->addWidget(m_knownTable);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    rootLayout->addWidget(splitter, 1);

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
}

void MainWindow::populateCatalogTree()
{
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
    QString error;
    m_knownDevices = m_store.load(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("读取失败"), error);
    }
    refreshKnownTable();
}

void MainWindow::refreshKnownTable()
{
    m_knownTable->setRowCount(m_knownDevices.size());
    for (int row = 0; row < m_knownDevices.size(); ++row) {
        const auto &device = m_knownDevices[row];
        const AcRemote *remote = findRemote(m_catalog, device.remoteId);
        m_knownTable->setItem(row, 0, new QTableWidgetItem(device.name));
        m_knownTable->setItem(row, 1, new QTableWidgetItem(device.brandId));
        m_knownTable->setItem(row, 2, new QTableWidgetItem(remote ? remote->name : device.remoteId));
        m_knownTable->setItem(row, 3, new QTableWidgetItem(QString::number(device.state.temp)));
    }
    m_knownTable->resizeColumnsToContents();
}

void MainWindow::refreshPorts()
{
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
}

void MainWindow::addAirConditioner()
{
    if (!m_serial.isOpen()) {
        QMessageBox::information(this, QStringLiteral("串口未连接"), QStringLiteral("请先连接 ESP32 串口。"));
        return;
    }

    AcDiscoveryWizard wizard(this, m_catalog);
    auto device = wizard.run([this](const QString &command) {
        return sendCommand(command);
    });
    if (!device) {
        return;
    }

    m_knownDevices.push_back(*device);

    QString error;
    if (!m_store.save(m_knownDevices, &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
    }
    refreshKnownTable();
    QMessageBox::information(this, QStringLiteral("已添加"), QStringLiteral("%1").arg(device->name));
}

void MainWindow::startKnownDevices()
{
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

void MainWindow::appendSerialLine(const QString &line)
{
    m_log->appendPlainText(QStringLiteral("%1  RX  %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), line));
}

void MainWindow::updateSerialStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

bool MainWindow::sendCommand(const QString &command)
{
    QString error;
    if (!m_serial.sendLine(command, &error)) {
        QMessageBox::warning(this, QStringLiteral("发送失败"), error);
        return false;
    }
    m_log->appendPlainText(QStringLiteral("%1  TX  %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), command));
    return true;
}
