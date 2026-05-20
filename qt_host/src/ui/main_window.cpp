#include "main_window.h"

#include "ac_control_dialog.h"
#include "ac_discovery_wizard.h"

#include <algorithm>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMouseEvent>
#include <QSpinBox>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStandardItem>
#include <QStyle>
#include <QSizePolicy>
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

QString sortModeName(int sortMode)
{
    switch (sortMode) {
    case 1:
        return QStringLiteral("品牌");
    case 2:
        return QStringLiteral("状态");
    case 3:
        return QStringLiteral("温度");
    case 0:
    default:
        return QStringLiteral("名称");
    }
}

QString makeCopyName(const QString &name)
{
    return name.isEmpty() ? QStringLiteral("空调 副本")
                          : QStringLiteral("%1 副本").arg(name);
}

constexpr int kCardWidth = 232;
constexpr int kCardHeight = 168;
constexpr int kCardSpacing = 12;
constexpr int kMinCardColumns = 2;
constexpr int kMaxCardColumns = 8;

QFrame *createInfoCell(QWidget *parent, const QString &label, const QString &value,
                       const QString &valueObjectName = QStringLiteral("cellValue"))
{
    auto *cell = new QFrame(parent);
    cell->setObjectName(QStringLiteral("infoCell"));
    cell->setAttribute(Qt::WA_TransparentForMouseEvents);
    cell->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    cell->setStyleSheet(QStringLiteral(
        "QFrame#infoCell {"
        "  background: #3b4656;"
        "  border: 1px solid #7dd3fc;"
        "  border-radius: 5px;"
        "}"
        "QLabel#cellLabel { color: #cbd5e1; font-size: 11px; }"
        "QLabel#cellValue { color: #f8fafc; font-size: 12px; font-weight: 600; }"
        "QLabel#stateOn { color: #22c55e; font-size: 12px; font-weight: 700; }"
        "QLabel#stateOff { color: #fb923c; font-size: 12px; font-weight: 700; }"));

    auto *layout = new QHBoxLayout(cell);
    layout->setContentsMargins(7, 4, 7, 4);
    layout->setSpacing(6);

    auto *labelView = new QLabel(label, cell);
    labelView->setObjectName(QStringLiteral("cellLabel"));
    labelView->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *valueView = new QLabel(value, cell);
    valueView->setObjectName(valueObjectName);
    valueView->setAttribute(Qt::WA_TransparentForMouseEvents);
    valueView->setWordWrap(false);
    valueView->setTextInteractionFlags(Qt::NoTextInteraction);
    valueView->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    valueView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    layout->addWidget(labelView);
    layout->addStretch(1);
    layout->addWidget(valueView);
    return cell;
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
    connect(&m_knownRunner, &KnownAcRunner::finished, this, [this]() {
        m_batchDeviceIndexes.clear();
        updateConnectionActions();
    });
    connect(&m_batchStateTimer, &QTimer::timeout, this, &MainWindow::sendNextBatchState);
    m_heartbeatTimer.setInterval(3000);
    m_batchStateTimer.setInterval(1500);
}

void MainWindow::buildUi()
{
    // 主界面采用上中下结构：
    // 顶部串口连接区，中部操作按钮和空调列表，底部串口日志。
    setWindowTitle(QStringLiteral("Universal ESP32 AC Remote"));
    resize(1280, 720);
    setMinimumWidth(kCardWidth * kMinCardColumns + kCardSpacing * 5);

    auto *central = new QWidget(this);
    central->setStyleSheet(QStringLiteral(
        "QWidget { background: #1f2937; color: #d7dee8; }"
        "QPushButton {"
        "  background: #334155;"
        "  border: 1px solid #475569;"
        "  border-radius: 6px;"
        "  color: #f8fafc;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover { background: #3f5168; }"
        "QPushButton:disabled { color: #7b8794; background: #253241; }"
        "QComboBox { background: #111827; border: 1px solid #475569; color: #f8fafc; padding: 4px 8px; }"
        "QPlainTextEdit { background: #111827; border: 1px solid #334155; color: #cbd5e1; }"
        "QScrollArea { background: #253241; border: 1px solid #334155; border-radius: 8px; }"));
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
    m_statusLabel->setStyleSheet(QStringLiteral("color: #cbd5e1;"));

    serialLayout->addWidget(m_portCombo);
    serialLayout->addWidget(m_refreshButton);
    serialLayout->addWidget(m_connectButton);
    serialLayout->addWidget(m_statusLabel);
    serialLayout->addStretch();
    rootLayout->addLayout(serialLayout);

    // 主操作区。38K/40K 测试按钮用于排查红外硬件和载波频率问题。
    auto *actionLayout = new QHBoxLayout;
    m_catalogButton = new QPushButton(style()->standardIcon(QStyle::SP_DirIcon), QStringLiteral("可选空调"), central);
    m_startButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), QStringLiteral("开启空调"), central);
    m_stopAllButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop), QStringLiteral("关闭所有空调"), central);
    m_addButton = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogNewFolder), QStringLiteral("添加空调"), central);
    m_renameButton = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogDetailedView), QStringLiteral("重命名"), central);
    m_deleteButton = new QPushButton(style()->standardIcon(QStyle::SP_TrashIcon), QStringLiteral("删除空调"), central);
    m_irTestButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton), QStringLiteral("红外测试"), central);
    m_logButton = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogDetailedView), QStringLiteral("串口日志"), central);
    m_batchSetButton = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogContentsView), QStringLiteral("批量设置"), central);
    m_stopTaskButton = new QPushButton(style()->standardIcon(QStyle::SP_BrowserStop), QStringLiteral("停止任务"), central);
    m_sortButton = new QPushButton(QStringLiteral("排序: 名称"), central);
    m_catalogButton->setMinimumHeight(48);
    m_startButton->setMinimumHeight(58);
    m_stopAllButton->setMinimumHeight(58);
    m_addButton->setMinimumHeight(58);
    m_renameButton->setMinimumHeight(58);
    m_deleteButton->setMinimumHeight(58);
    m_irTestButton->setMinimumHeight(58);
    m_logButton->setMinimumHeight(58);
    m_batchSetButton->setMinimumHeight(58);
    m_stopTaskButton->setMinimumHeight(58);
    m_sortButton->setMinimumHeight(58);
    m_catalogButton->setIconSize(QSize(24, 24));
    m_startButton->setIconSize(QSize(28, 28));
    m_stopAllButton->setIconSize(QSize(28, 28));
    m_addButton->setIconSize(QSize(28, 28));
    m_renameButton->setIconSize(QSize(24, 24));
    m_deleteButton->setIconSize(QSize(24, 24));
    m_irTestButton->setIconSize(QSize(24, 24));
    m_logButton->setIconSize(QSize(24, 24));
    m_batchSetButton->setIconSize(QSize(24, 24));
    m_stopTaskButton->setIconSize(QSize(24, 24));
    m_catalogButton->setToolTip(QStringLiteral("查看固件返回的品牌和候选遥控器"));
    m_startButton->setToolTip(QStringLiteral("有勾选时只开启已选空调，未勾选时开启全部"));
    m_stopAllButton->setToolTip(QStringLiteral("按间隔逐台发送关机命令"));
    m_renameButton->setToolTip(QStringLiteral("重命名当前选中的空调"));
    m_deleteButton->setToolTip(QStringLiteral("从已保存空调库中删除当前空调"));
    m_irTestButton->setToolTip(QStringLiteral("选择 38kHz 或 40kHz NEC 红外测试帧"));
    m_logButton->setToolTip(QStringLiteral("查看串口 TX/RX 日志"));
    m_batchSetButton->setToolTip(QStringLiteral("统一设置已勾选空调的温度、模式、风速和摆风"));
    m_stopTaskButton->setToolTip(QStringLiteral("停止后续批量命令发送"));
    m_sortButton->setToolTip(QStringLiteral("切换空调卡片排序方式"));
    actionLayout->addWidget(m_catalogButton);
    actionLayout->addWidget(m_startButton);
    actionLayout->addWidget(m_stopAllButton);
    actionLayout->addWidget(m_addButton);
    actionLayout->addWidget(m_renameButton);
    actionLayout->addWidget(m_deleteButton);
    actionLayout->addWidget(m_batchSetButton);
    actionLayout->addWidget(m_stopTaskButton);
    actionLayout->addWidget(m_irTestButton);
    actionLayout->addWidget(m_logButton);
    actionLayout->addStretch();
    rootLayout->addLayout(actionLayout);

    auto *filterLayout = new QHBoxLayout;
    m_searchEdit = new QLineEdit(central);
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索名称、品牌、遥控器"));
    m_searchEdit->setMinimumHeight(36);
    m_searchEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #111827; border: 1px solid #475569; color: #f8fafc; padding: 6px 10px; border-radius: 6px; }"));
    m_selectionLabel = new QLabel(QStringLiteral("未选择"), central);
    m_selectionLabel->setMinimumWidth(100);
    m_selectionLabel->setStyleSheet(QStringLiteral("color: #cbd5e1;"));
    m_selectAllButton = new QPushButton(QStringLiteral("全选"), central);
    m_clearSelectionButton = new QPushButton(QStringLiteral("清空选择"), central);
    filterLayout->addWidget(m_searchEdit, 1);
    filterLayout->addWidget(m_sortButton);
    filterLayout->addWidget(m_selectionLabel);
    filterLayout->addWidget(m_selectAllButton);
    filterLayout->addWidget(m_clearSelectionButton);
    rootLayout->addLayout(filterLayout);

    // 可选遥控器目录只在用户点击“可选空调”时弹出，主界面把空间留给空调卡片。
    m_catalogModel = new QStandardItemModel(this);

    m_knownScrollArea = new QScrollArea(central);
    m_knownScrollArea->setWidgetResizable(true);
    m_knownScrollArea->setFrameShape(QFrame::NoFrame);
    m_knownScrollArea->viewport()->setStyleSheet(QStringLiteral("background: #253241;"));
    m_knownCardContainer = new QWidget(m_knownScrollArea);
    m_knownCardContainer->setStyleSheet(QStringLiteral("background: #253241;"));
    m_knownCardLayout = new QGridLayout(m_knownCardContainer);
    m_knownCardLayout->setContentsMargins(12, 12, 12, 12);
    m_knownCardLayout->setSpacing(kCardSpacing);
    m_knownCardLayout->setAlignment(Qt::AlignTop);
    m_knownScrollArea->setWidget(m_knownCardContainer);
    rootLayout->addWidget(m_knownScrollArea, 1);

    // 串口日志不常驻主界面，点击“串口日志”时弹窗查看。
    m_logDialog = new QDialog(this);
    m_logDialog->setWindowTitle(QStringLiteral("串口日志"));
    m_logDialog->resize(760, 360);
    m_logDialog->setStyleSheet(QStringLiteral(
        "QDialog { background: #1f2937; color: #d7dee8; }"
        "QPlainTextEdit { background: #111827; border: 1px solid #334155; color: #cbd5e1; }"));
    auto *logLayout = new QVBoxLayout(m_logDialog);
    m_log = new QPlainTextEdit(m_logDialog);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(400);
    logLayout->addWidget(m_log);
    auto *logButtonLayout = new QHBoxLayout;
    logButtonLayout->addStretch();
    m_clearLogButton = new QPushButton(QStringLiteral("清空日志"), m_logDialog);
    m_copyLogButton = new QPushButton(QStringLiteral("复制日志"), m_logDialog);
    logButtonLayout->addWidget(m_clearLogButton);
    logButtonLayout->addWidget(m_copyLogButton);
    logLayout->addLayout(logButtonLayout);

    setCentralWidget(central);

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(m_catalogButton, &QPushButton::clicked, this, &MainWindow::showCatalogPopup);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startKnownDevices);
    connect(m_stopAllButton, &QPushButton::clicked, this, &MainWindow::stopKnownDevices);
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addAirConditioner);
    connect(m_renameButton, &QPushButton::clicked, this, &MainWindow::renameAirConditioner);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteAirConditioner);
    connect(m_batchSetButton, &QPushButton::clicked, this, &MainWindow::batchSetSelectedDevices);
    connect(m_stopTaskButton, &QPushButton::clicked, this, &MainWindow::stopBatchTask);
    connect(m_irTestButton, &QPushButton::clicked, this, &MainWindow::showIrTestMenu);
    connect(m_logButton, &QPushButton::clicked, this, &MainWindow::showLogWindow);
    connect(m_sortButton, &QPushButton::clicked, this, &MainWindow::showSortMenu);
    connect(m_selectAllButton, &QPushButton::clicked, this, &MainWindow::selectAllVisibleDevices);
    connect(m_clearSelectionButton, &QPushButton::clicked, this, &MainWindow::clearSelectedDevices);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_searchText = text.trimmed();
        rebuildKnownCardGrid();
        updateSelectionControls();
    });
    connect(m_clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLog);
    connect(m_copyLogButton, &QPushButton::clicked, this, &MainWindow::copyLog);
    updateConnectionActions();
    updateSelectionControls();
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
}

void MainWindow::showCatalogPopup()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("可选空调"));
    dialog.resize(460, 520);
    dialog.setStyleSheet(QStringLiteral(
        "QDialog { background: #1f2937; color: #d7dee8; }"
        "QTreeView { background: #111827; border: 1px solid #334155; color: #e5edf6; }"
        "QPushButton { background: #334155; border: 1px solid #475569; border-radius: 6px; color: #f8fafc; padding: 6px 12px; }"));

    auto *layout = new QVBoxLayout(&dialog);
    auto *tree = new QTreeView(&dialog);
    tree->setModel(m_catalogModel);
    tree->setHeaderHidden(true);
    tree->expandAll();
    layout->addWidget(tree, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::accept);
    layout->addWidget(buttons);
    dialog.exec();
}

void MainWindow::showIrTestMenu()
{
    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(
        "QMenu { background: #111827; color: #f8fafc; border: 1px solid #334155; }"
        "QMenu::item:selected { background: #2563eb; }"));
    QAction *test38 = menu.addAction(QStringLiteral("38K 测试"));
    QAction *test40 = menu.addAction(QStringLiteral("40K 测试"));
    connect(test38, &QAction::triggered, this, &MainWindow::testIr38k);
    connect(test40, &QAction::triggered, this, &MainWindow::testIr40k);
    menu.exec(m_irTestButton->mapToGlobal(QPoint(0, m_irTestButton->height())));
}

void MainWindow::showLogWindow()
{
    if (!m_logDialog) {
        return;
    }
    m_logDialog->show();
    m_logDialog->raise();
    m_logDialog->activateWindow();
}

void MainWindow::showSortMenu()
{
    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(
        "QMenu { background: #111827; color: #f8fafc; border: 1px solid #334155; }"
        "QMenu::item:selected { background: #2563eb; }"));
    const QStringList labels = {
        QStringLiteral("名称"),
        QStringLiteral("品牌"),
        QStringLiteral("状态"),
        QStringLiteral("温度"),
    };
    for (int mode = 0; mode < labels.size(); ++mode) {
        QAction *action = menu.addAction(labels[mode]);
        action->setCheckable(true);
        action->setChecked(m_sortMode == mode);
        connect(action, &QAction::triggered, this, [this, mode]() {
            m_sortMode = mode;
            m_sortButton->setText(QStringLiteral("排序: %1").arg(sortModeName(m_sortMode)));
            rebuildKnownCardGrid();
        });
    }
    menu.exec(m_sortButton->mapToGlobal(QPoint(0, m_sortButton->height())));
}

void MainWindow::selectAllVisibleDevices()
{
    for (int index : visibleDeviceIndexes()) {
        m_selectedDeviceIds.insert(m_knownDevices[index].id);
    }
    rebuildKnownCardGrid();
    updateSelectionControls();
}

void MainWindow::clearSelectedDevices()
{
    m_selectedDeviceIds.clear();
    rebuildKnownCardGrid();
    updateSelectionControls();
}

void MainWindow::stopBatchTask()
{
    m_knownRunner.stop();
    m_batchStateTimer.stop();
    m_batchDeviceIndexes.clear();
    m_batchStates.clear();
    updateConnectionActions();
}

void MainWindow::clearLog()
{
    if (m_log) {
        m_log->clear();
    }
}

void MainWindow::copyLog()
{
    if (m_log) {
        QApplication::clipboard()->setText(m_log->toPlainText());
    }
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
    updateSelectionControls();
}

bool MainWindow::deviceMatchesSearch(int index) const
{
    if (m_searchText.isEmpty()) {
        return true;
    }
    if (index < 0 || index >= m_knownDevices.size()) {
        return false;
    }

    const KnownAcDevice &device = m_knownDevices[index];
    const AcRemote *remote = findRemote(m_catalog, device.remoteId);
    const QString haystack = QStringLiteral("%1 %2 %3 %4")
                                 .arg(device.name, device.brandId, device.remoteId,
                                      remote ? remote->name : QString())
                                 .toLower();
    return haystack.contains(m_searchText.toLower());
}

QVector<int> MainWindow::visibleDeviceIndexes() const
{
    QVector<int> indexes;
    for (int i = 0; i < m_knownDevices.size(); ++i) {
        if (deviceMatchesSearch(i)) {
            indexes.push_back(i);
        }
    }

    std::sort(indexes.begin(), indexes.end(), [this](int lhs, int rhs) {
        const KnownAcDevice &a = m_knownDevices[lhs];
        const KnownAcDevice &b = m_knownDevices[rhs];
        switch (m_sortMode) {
        case 1:
            return a.brandId.localeAwareCompare(b.brandId) < 0;
        case 2:
            if (a.state.power != b.state.power) {
                return a.state.power && !b.state.power;
            }
            return a.name.localeAwareCompare(b.name) < 0;
        case 3:
            if (a.state.temp != b.state.temp) {
                return a.state.temp < b.state.temp;
            }
            return a.name.localeAwareCompare(b.name) < 0;
        case 0:
        default:
            return a.name.localeAwareCompare(b.name) < 0;
        }
    });
    return indexes;
}

QVector<int> MainWindow::selectedDeviceIndexes() const
{
    QVector<int> indexes;
    for (int i = 0; i < m_knownDevices.size(); ++i) {
        if (m_selectedDeviceIds.contains(m_knownDevices[i].id)) {
            indexes.push_back(i);
        }
    }
    return indexes;
}

void MainWindow::updateSelectionControls()
{
    const int selectedCount = selectedDeviceIndexes().size();
    if (m_selectionLabel) {
        m_selectionLabel->setText(selectedCount > 0
                                      ? QStringLiteral("已选 %1 台").arg(selectedCount)
                                      : QStringLiteral("未选择"));
    }
    const bool hasDevices = !m_knownDevices.isEmpty();
    const bool hasSelection = selectedCount > 0;
    if (m_selectAllButton) {
        m_selectAllButton->setEnabled(hasDevices && !visibleDeviceIndexes().isEmpty());
    }
    if (m_clearSelectionButton) {
        m_clearSelectionButton->setEnabled(hasSelection);
    }
    if (m_batchSetButton) {
        m_batchSetButton->setEnabled(hasSelection && m_serial.isOpen());
    }
    if (m_renameButton) {
        m_renameButton->setEnabled(m_selectedDeviceIndex >= 0 &&
                                   m_selectedDeviceIndex < m_knownDevices.size());
    }
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
    card->setFixedSize(kCardWidth, kCardHeight);
    card->setProperty("deviceIndex", index);
    card->setContextMenuPolicy(Qt::CustomContextMenu);
    card->setStyleSheet(QStringLiteral(
        "QFrame#knownAcCard {"
        "  border: 2px solid %1;"
        "  border-radius: 8px;"
        "  background: %2;"
        "}"
        "QLabel#title { color: #f8fafc; font-size: 15px; font-weight: 700; }"
        "QLabel#remoteInfo { color: #cbd5e1; font-size: 11px; }")
            .arg(selected ? QStringLiteral("#38bdf8") : QStringLiteral("#7dd3fc"),
                 QStringLiteral("#334155")));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(7);

    auto *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(6);
    auto *check = new QCheckBox(card);
    check->setChecked(m_selectedDeviceIds.contains(device.id));
    check->setToolTip(QStringLiteral("选择这台空调"));
    check->setStyleSheet(QStringLiteral("QCheckBox { background: transparent; }"));
    auto *title = new QLabel(device.name.isEmpty() ? device.remoteId : device.name, card);
    title->setObjectName(QStringLiteral("title"));
    title->setWordWrap(false);
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    title->setToolTip(title->text());
    title->setText(title->fontMetrics().elidedText(title->text(), Qt::ElideRight,
                                                   kCardWidth - 52));
    topLayout->addWidget(check);
    topLayout->addWidget(title, 1);
    layout->addLayout(topLayout);

    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(6);
    grid->addWidget(createInfoCell(card, QStringLiteral("状态"),
                                   device.state.power ? QStringLiteral("已开启")
                                                      : QStringLiteral("已关闭"),
                                   device.state.power ? QStringLiteral("stateOn")
                                                      : QStringLiteral("stateOff")),
                    0, 0);
    grid->addWidget(createInfoCell(card, QStringLiteral("温度"),
                                   QStringLiteral("%1°C").arg(device.state.temp)),
                    0, 1);
    grid->addWidget(createInfoCell(card, QStringLiteral("模式"),
                                   modeDisplayName(device.state.mode)),
                    1, 0);
    grid->addWidget(createInfoCell(card, QStringLiteral("风速"),
                                   fanDisplayName(device.state.fan)),
                    1, 1);
    grid->addWidget(createInfoCell(card, QStringLiteral("上下风"),
                                   swingDisplayName(device.state.swingv)),
                    2, 0);
    grid->addWidget(createInfoCell(card, QStringLiteral("左右风"),
                                   swingDisplayName(device.state.swingh)),
                    2, 1);
    layout->addLayout(grid);

    auto *remoteInfo = new QLabel(
        QStringLiteral("%1 / %2")
            .arg(device.brandId, remote ? remote->name : device.remoteId),
        card);
    remoteInfo->setObjectName(QStringLiteral("remoteInfo"));
    remoteInfo->setWordWrap(false);
    remoteInfo->setAttribute(Qt::WA_TransparentForMouseEvents);
    remoteInfo->setToolTip(remoteInfo->text());
    remoteInfo->setText(remoteInfo->fontMetrics().elidedText(remoteInfo->text(),
                                                             Qt::ElideRight,
                                                             kCardWidth - 24));
    layout->addWidget(remoteInfo);
    layout->addStretch();

    card->installEventFilter(this);
    connect(check, &QCheckBox::toggled, this, [this, id = device.id](bool checked) {
        if (checked) {
            m_selectedDeviceIds.insert(id);
        } else {
            m_selectedDeviceIds.remove(id);
        }
        rebuildKnownCardGrid();
        updateSelectionControls();
    });
    connect(card, &QFrame::customContextMenuRequested, this, [this, card](const QPoint &pos) {
        const int index = card->property("deviceIndex").toInt();
        if (index >= 0 && index < m_knownDevices.size()) {
            showCardContextMenu(index, card->mapToGlobal(pos));
        }
    });
    return card;
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
    const int cellWidth = kCardWidth + kCardSpacing;
    const int columns = qBound(kMinCardColumns,
                               qMax(kMinCardColumns, width / cellWidth),
                               kMaxCardColumns);
    for (int column = 0; column < kMaxCardColumns; ++column) {
        m_knownCardLayout->setColumnMinimumWidth(column, 0);
        m_knownCardLayout->setColumnStretch(column, 0);
    }
    const QVector<int> indexes = visibleDeviceIndexes();
    for (int visible = 0; visible < indexes.size(); ++visible) {
        m_knownCardLayout->addWidget(createKnownDeviceCard(indexes[visible]),
                                     visible / columns, visible % columns);
    }
    for (int column = 0; column < columns; ++column) {
        m_knownCardLayout->setColumnMinimumWidth(column, kCardWidth);
        m_knownCardLayout->setColumnStretch(column, 0);
    }
}

void MainWindow::updateConnectionActions()
{
    const bool connected = m_serial.isOpen();
    m_startButton->setEnabled(connected);
    m_stopAllButton->setEnabled(connected);
    m_addButton->setEnabled(connected);
    m_renameButton->setEnabled(m_selectedDeviceIndex >= 0 &&
                               m_selectedDeviceIndex < m_knownDevices.size());
    m_irTestButton->setEnabled(connected);
    m_stopTaskButton->setEnabled(m_knownRunner.isRunning() || m_batchStateTimer.isActive());
    if (m_batchSetButton) {
        m_batchSetButton->setEnabled(connected && !selectedDeviceIndexes().isEmpty());
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonDblClick) {
        if (auto *card = qobject_cast<QFrame *>(watched)) {
            const int index = card->property("deviceIndex").toInt();
            if (index >= 0 && index < m_knownDevices.size()) {
                if (auto *mouseEvent = dynamic_cast<QMouseEvent *>(event)) {
                    if (mouseEvent->button() != Qt::LeftButton) {
                        m_selectedDeviceIndex = index;
                        updateSelectionControls();
                        return false;
                    }
                }
                m_selectedDeviceIndex = index;
                rebuildKnownCardGrid();
                updateSelectionControls();
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

void MainWindow::renameAirConditioner()
{
    const int row = m_selectedDeviceIndex;
    if (row < 0 || row >= m_knownDevices.size()) {
        QMessageBox::information(this, QStringLiteral("未选择空调"),
                                 QStringLiteral("请先在空调库中选择一个空调。"));
        return;
    }
    renameDevice(row);
}

void MainWindow::deleteAirConditioner()
{
    // 删除只影响电脑端 JSON，不会改 ESP32 内部状态。
    const QVector<int> selected = selectedDeviceIndexes();
    if (!selected.isEmpty()) {
        const auto reply = QMessageBox::question(
            this,
            QStringLiteral("删除空调"),
            QStringLiteral("确定删除已选的 %1 台空调？").arg(selected.size()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }

        for (int i = selected.size() - 1; i >= 0; --i) {
            m_knownDevices.removeAt(selected[i]);
        }
        m_selectedDeviceIds.clear();
        m_selectedDeviceIndex = m_knownDevices.isEmpty() ? -1 : qMin(m_selectedDeviceIndex, m_knownDevices.size() - 1);
        if (!saveKnownDevices()) {
            return;
        }
        refreshKnownCards();
        return;
    }

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

    QVector<int> targetIndexes = selectedDeviceIndexes();
    if (targetIndexes.isEmpty()) {
        for (int i = 0; i < m_knownDevices.size(); ++i) {
            targetIndexes.push_back(i);
        }
    }

    QVector<KnownAcDevice> targets;
    targets.reserve(targetIndexes.size());
    for (int index : targetIndexes) {
        targets.push_back(m_knownDevices[index]);
    }
    m_batchDeviceIndexes = targetIndexes;

    m_knownRunner.start(
        targets,
        targetPower,
        [this](const QString &command) {
            return sendCommand(command);
        },
        [this](int index, const AcState &state) {
            if (index < 0 || index >= m_batchDeviceIndexes.size()) {
                return;
            }
            const int originalIndex = m_batchDeviceIndexes[index];
            if (originalIndex < 0 || originalIndex >= m_knownDevices.size()) {
                return;
            }
            m_knownDevices[originalIndex].state = state;
            saveKnownDevices();
            refreshKnownCards();
        });
    updateConnectionActions();
}

void MainWindow::renameDevice(int index)
{
    if (index < 0 || index >= m_knownDevices.size()) {
        return;
    }
    bool accepted = false;
    const QString name = QInputDialog::getText(
        this, QStringLiteral("重命名空调"), QStringLiteral("名称"),
        QLineEdit::Normal, m_knownDevices[index].name, &accepted);
    if (!accepted || name.trimmed().isEmpty()) {
        return;
    }
    m_knownDevices[index].name = name.trimmed();
    if (saveKnownDevices()) {
        refreshKnownCards();
    }
}

void MainWindow::duplicateDevice(int index)
{
    if (index < 0 || index >= m_knownDevices.size()) {
        return;
    }
    KnownAcDevice copy = m_knownDevices[index];
    copy.id = QStringLiteral("%1_copy_%2")
                  .arg(copy.id)
                  .arg(QDateTime::currentMSecsSinceEpoch());
    copy.name = makeCopyName(copy.name);
    m_knownDevices.push_back(copy);
    m_selectedDeviceIndex = m_knownDevices.size() - 1;
    if (saveKnownDevices()) {
        refreshKnownCards();
    }
}

void MainWindow::deleteDeviceAt(int index)
{
    if (index < 0 || index >= m_knownDevices.size()) {
        return;
    }
    const auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除空调"),
        QStringLiteral("确定删除“%1”？").arg(m_knownDevices[index].name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    m_selectedDeviceIds.remove(m_knownDevices[index].id);
    m_knownDevices.removeAt(index);
    m_selectedDeviceIndex = m_knownDevices.isEmpty() ? -1 : qMin(index, m_knownDevices.size() - 1);
    if (saveKnownDevices()) {
        refreshKnownCards();
    }
}

void MainWindow::showCardContextMenu(int index, const QPoint &globalPos)
{
    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(
        "QMenu { background: #111827; color: #f8fafc; border: 1px solid #334155; }"
        "QMenu::item:selected { background: #2563eb; }"));
    QAction *details = menu.addAction(QStringLiteral("详情"));
    QAction *rename = menu.addAction(QStringLiteral("重命名"));
    QAction *duplicate = menu.addAction(QStringLiteral("复制"));
    QAction *remove = menu.addAction(QStringLiteral("删除"));
    connect(details, &QAction::triggered, this, [this, index]() { openKnownDeviceControl(index); });
    connect(rename, &QAction::triggered, this, [this, index]() { renameDevice(index); });
    connect(duplicate, &QAction::triggered, this, [this, index]() { duplicateDevice(index); });
    connect(remove, &QAction::triggered, this, [this, index]() { deleteDeviceAt(index); });
    menu.exec(globalPos);
}

void MainWindow::batchSetSelectedDevices()
{
    const QVector<int> indexes = selectedDeviceIndexes();
    if (indexes.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择空调"),
                                 QStringLiteral("请先勾选要批量设置的空调。"));
        return;
    }
    if (!m_serial.isOpen()) {
        QMessageBox::information(this, QStringLiteral("串口未连接"),
                                 QStringLiteral("请先连接 ESP32 串口。"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("批量设置"));
    dialog.setStyleSheet(QStringLiteral(
        "QDialog { background: #1f2937; color: #d7dee8; }"
        "QComboBox, QSpinBox { background: #111827; border: 1px solid #475569; color: #f8fafc; padding: 4px 8px; }"
        "QPushButton { background: #334155; border: 1px solid #475569; border-radius: 6px; color: #f8fafc; padding: 6px 12px; }"
        "QCheckBox { color: #d7dee8; }"));
    auto *layout = new QGridLayout(&dialog);

    auto *tempSpin = new QSpinBox(&dialog);
    tempSpin->setRange(16, 32);
    tempSpin->setValue(26);
    auto *modeCombo = new QComboBox(&dialog);
    modeCombo->addItem(QStringLiteral("自动"), QStringLiteral("auto"));
    modeCombo->addItem(QStringLiteral("制冷"), QStringLiteral("cool"));
    modeCombo->addItem(QStringLiteral("制热"), QStringLiteral("heat"));
    modeCombo->addItem(QStringLiteral("除湿"), QStringLiteral("dry"));
    modeCombo->addItem(QStringLiteral("送风"), QStringLiteral("fan"));
    auto *fanCombo = new QComboBox(&dialog);
    fanCombo->addItem(QStringLiteral("自动"), QStringLiteral("auto"));
    fanCombo->addItem(QStringLiteral("低风"), QStringLiteral("low"));
    fanCombo->addItem(QStringLiteral("中风"), QStringLiteral("med"));
    fanCombo->addItem(QStringLiteral("高风"), QStringLiteral("high"));
    fanCombo->addItem(QStringLiteral("强风"), QStringLiteral("max"));
    auto *swingVCheck = new QCheckBox(QStringLiteral("上下风"), &dialog);
    auto *swingHCheck = new QCheckBox(QStringLiteral("左右风"), &dialog);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

    layout->addWidget(new QLabel(QStringLiteral("温度"), &dialog), 0, 0);
    layout->addWidget(tempSpin, 0, 1);
    layout->addWidget(new QLabel(QStringLiteral("模式"), &dialog), 1, 0);
    layout->addWidget(modeCombo, 1, 1);
    layout->addWidget(new QLabel(QStringLiteral("风速"), &dialog), 2, 0);
    layout->addWidget(fanCombo, 2, 1);
    layout->addWidget(swingVCheck, 3, 0, 1, 2);
    layout->addWidget(swingHCheck, 4, 0, 1, 2);
    layout->addWidget(buttons, 5, 0, 1, 2);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_batchDeviceIndexes = indexes;
    m_batchStates.clear();
    for (int index : indexes) {
        AcState state = m_knownDevices[index].state;
        state.power = true;
        state.temp = tempSpin->value();
        state.mode = modeCombo->currentData().toString();
        state.fan = fanCombo->currentData().toString();
        state.swingv = swingVCheck->isChecked() ? QStringLiteral("auto") : QStringLiteral("off");
        state.swingh = swingHCheck->isChecked() ? QStringLiteral("auto") : QStringLiteral("off");
        m_batchStates.push_back(state);
    }
    sendNextBatchState();
    if (!m_batchStates.isEmpty()) {
        m_batchStateTimer.start();
    }
    updateConnectionActions();
}

void MainWindow::sendNextBatchState()
{
    while (!m_batchDeviceIndexes.isEmpty() && !m_batchStates.isEmpty()) {
        const int index = m_batchDeviceIndexes.takeFirst();
        const AcState state = m_batchStates.takeFirst();
        if (index < 0 || index >= m_knownDevices.size()) {
            continue;
        }
        if (!sendCommand(buildAcCommand(m_knownDevices[index].remoteId, state))) {
            m_batchStateTimer.stop();
            m_batchDeviceIndexes.clear();
            m_batchStates.clear();
            updateConnectionActions();
            return;
        }
        m_knownDevices[index].state = state;
        saveKnownDevices();
        refreshKnownCards();
        break;
    }

    if (m_batchDeviceIndexes.isEmpty() || m_batchStates.isEmpty()) {
        m_batchStateTimer.stop();
        m_batchDeviceIndexes.clear();
        m_batchStates.clear();
        updateConnectionActions();
    }
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
    m_batchStateTimer.stop();
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
