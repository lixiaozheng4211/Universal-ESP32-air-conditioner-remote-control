#include "ac_control_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStyle>
#include <QVBoxLayout>
#include <utility>

namespace {

// QComboBox 里显示中文标签，但 data 保存固件协议使用的英文值。
// 这样 UI 文案可以本地化，串口命令仍保持稳定。
void addMode(QComboBox* combo, const QString& value, const QString& label) {
  combo->addItem(label, value);
}

}  // namespace

AcControlDialog::AcControlDialog(QWidget* parent, const KnownAcDevice& device,
                                 const AcRemote* remote, SendAction sendAction)
    : QDialog(parent),
      m_device(device),
      m_remote(remote),
      m_sendAction(std::move(sendAction)),
      m_state(device.state) {
  buildUi();
  syncUiFromState();
}

AcState AcControlDialog::state() const { return m_state; }

void AcControlDialog::buildUi() {
  // 详情页保持轻量：开关机是立即动作，温度/模式/上下风需要点击“发送控制”。
  // 这样用户可以先调整多个控件，再由 sendUiState 按固定顺序逐项发送。
  setWindowTitle(QStringLiteral("空调控制"));
  setMinimumWidth(360);

  auto* rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(16, 16, 16, 16);
  rootLayout->setSpacing(12);

  m_titleLabel = new QLabel(m_device.name, this);
  QFont titleFont = m_titleLabel->font();
  titleFont.setPointSize(titleFont.pointSize() + 2);
  titleFont.setBold(true);
  m_titleLabel->setFont(titleFont);
  rootLayout->addWidget(m_titleLabel);

  const QString remoteName = m_remote ? m_remote->name : m_device.remoteId;
  auto* remoteLabel = new QLabel(
      QStringLiteral("%1 / %2").arg(m_device.brandId, remoteName), this);
  rootLayout->addWidget(remoteLabel);

  auto* powerLayout = new QHBoxLayout;
  m_powerOnButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay),
                                    QStringLiteral("开机"), this);
  m_powerOffButton =
      new QPushButton(style()->standardIcon(QStyle::SP_MediaStop),
                      QStringLiteral("关机"), this);
  m_powerOnButton->setMinimumHeight(40);
  m_powerOffButton->setMinimumHeight(40);
  powerLayout->addWidget(m_powerOnButton);
  powerLayout->addWidget(m_powerOffButton);
  rootLayout->addLayout(powerLayout);

  auto* formLayout = new QFormLayout;
  formLayout->setLabelAlignment(Qt::AlignRight);

  m_tempSpin = new QSpinBox(this);
  m_tempSpin->setRange(m_remote ? m_remote->minTemp : 16,
                       m_remote ? m_remote->maxTemp : 30);
  m_tempSpin->setSuffix(QStringLiteral(" °C"));
  m_tempSpin->setMinimumHeight(34);
  formLayout->addRow(QStringLiteral("温度"), m_tempSpin);

  m_modeCombo = new QComboBox(this);
  addMode(m_modeCombo, QStringLiteral("auto"), QStringLiteral("自动"));
  addMode(m_modeCombo, QStringLiteral("cool"), QStringLiteral("制冷"));
  addMode(m_modeCombo, QStringLiteral("heat"), QStringLiteral("制热"));
  addMode(m_modeCombo, QStringLiteral("dry"), QStringLiteral("除湿"));
  addMode(m_modeCombo, QStringLiteral("fan"), QStringLiteral("送风"));
  m_modeCombo->setMinimumHeight(34);
  formLayout->addRow(QStringLiteral("模式"), m_modeCombo);

  m_swingVCheck = new QCheckBox(QStringLiteral("上下风"), this);
  if (m_remote && !m_remote->supportsSwingV) {
    m_swingVCheck->setEnabled(false);
  }
  formLayout->addRow(QString(), m_swingVCheck);
  rootLayout->addLayout(formLayout);

  m_sendButton =
      new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton),
                      QStringLiteral("发送控制"), this);
  m_sendButton->setMinimumHeight(40);
  rootLayout->addWidget(m_sendButton);

  m_statusLabel = new QLabel(QString(), this);
  rootLayout->addWidget(m_statusLabel);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  rootLayout->addWidget(buttons);

  connect(m_powerOnButton, &QPushButton::clicked, this,
          [this]() { setPowerAndSend(true); });
  connect(m_powerOffButton, &QPushButton::clicked, this,
          [this]() { setPowerAndSend(false); });
  connect(m_sendButton, &QPushButton::clicked, this,
          &AcControlDialog::sendUiState);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
}

void AcControlDialog::syncUiFromState() {
  // 只把已经发送成功并保存在 m_state 里的内容写回 UI。
  // 如果用户在界面上改了值但发送失败，这里不会误把失败状态当成真实状态。
  m_tempSpin->setValue(m_state.temp);

  const int modeIndex = m_modeCombo->findData(m_state.mode);
  m_modeCombo->setCurrentIndex(
      modeIndex >= 0 ? modeIndex
                     : m_modeCombo->findData(QStringLiteral("cool")));

  m_swingVCheck->setChecked(m_state.swingv == QStringLiteral("auto"));
}

AcState AcControlDialog::uiState() const {
  // 以 m_state 为基础，是为了保留当前未在界面展示的字段，例如 fan、eco。
  // 当前详情页只开放温度、模式、上下风，其它功能先保持原值或固定关闭。
  AcState state = m_state;
  state.temp = m_tempSpin->value();
  state.mode = m_modeCombo->currentData().toString();
  state.swingv =
      (m_remote && m_remote->supportsSwingV && m_swingVCheck->isChecked())
          ? QStringLiteral("auto")
          : QStringLiteral("off");
  state.swingh = QStringLiteral("off");
  state.eco = false;
  return state;
}

bool AcControlDialog::sendUiState() {
  AcState nextState = uiState();
  AcState workingState = m_state;
  int sentCount = 0;

  // 每个步骤成功后立即更新 workingState 和 m_state。
  // 如果中途失败，前面已经成功的状态会被保留，后续未成功的变化不会写入本地库。
  auto sendStep = [this, &workingState, &sentCount](const QString& action,
                                                    const AcState& stepState) {
    if (!m_sendAction || !m_sendAction(action, stepState)) {
      return false;
    }
    workingState = stepState;
    m_state = workingState;
    ++sentCount;
    return true;
  };

  // 固定顺序和固件协议保持一致：power -> mode -> temp -> swingv。
  // 对 RN02S13 这种非完整状态遥控器，每一步都会变成一条独立红外码。
  if (nextState.power != workingState.power) {
    AcState stepState = workingState;
    stepState.power = nextState.power;
    if (!sendStep(QStringLiteral("power"), stepState)) {
      m_statusLabel->setText(QStringLiteral("发送失败"));
      return false;
    }
  }
  if (nextState.mode != workingState.mode) {
    AcState stepState = workingState;
    stepState.mode = nextState.mode;
    if (!sendStep(QStringLiteral("mode"), stepState)) {
      m_statusLabel->setText(QStringLiteral("发送失败"));
      return false;
    }
  }
  if (nextState.temp != workingState.temp) {
    AcState stepState = workingState;
    stepState.temp = nextState.temp;
    if (!sendStep(QStringLiteral("temp"), stepState)) {
      m_statusLabel->setText(QStringLiteral("发送失败"));
      return false;
    }
  }
  if (nextState.swingv != workingState.swingv) {
    AcState stepState = workingState;
    stepState.swingv = nextState.swingv;
    if (!sendStep(QStringLiteral("swingv"), stepState)) {
      m_statusLabel->setText(QStringLiteral("发送失败"));
      return false;
    }
  }

  if (sentCount == 0) {
    m_statusLabel->setText(QStringLiteral("无变化"));
    return true;
  }

  syncUiFromState();
  m_statusLabel->setText(QStringLiteral("已发送 %1 项").arg(sentCount));
  return true;
}

void AcControlDialog::setPowerAndSend(bool power) {
  // 开关机按钮是单项动作，只从 m_state 改 power。
  // 不能调用 uiState()，否则用户尚未点击“发送控制”的温度/模式也会被误保存。
  AcState nextState = m_state;
  nextState.power = power;
  if (!m_sendAction || !m_sendAction(QStringLiteral("power"), nextState)) {
    m_statusLabel->setText(QStringLiteral("发送失败"));
    return;
  }

  m_state = nextState;
  syncUiFromState();
  m_statusLabel->setText(power ? QStringLiteral("已开机")
                               : QStringLiteral("已关机"));
}
