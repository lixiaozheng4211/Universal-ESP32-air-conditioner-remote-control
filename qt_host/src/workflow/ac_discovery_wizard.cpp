#include "ac_discovery_wizard.h"

#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QUuid>

AcDiscoveryWizard::AcDiscoveryWizard(QWidget* parent,
                                     const QVector<AcBrand>& catalog)
    : m_parent(parent), m_catalog(catalog) {}

std::optional<KnownAcDevice> AcDiscoveryWizard::run(
    const SendCommand& sendCommand) {
  // 用户先选品牌，后续只遍历该品牌下的遥控器候选，避免跨品牌盲试太慢。
  const AcBrand* brand = selectBrand();
  if (!brand) {
    return std::nullopt;
  }

  for (const auto& remote : brand->remotes) {
    // 第一步：发送完整开机状态。添加流程使用完整状态，是为了提高匹配判断的确定性。
    // 详情控制页才会使用 action=power/temp 这种单项命令。
    AcState onState;
    onState.power = true;
    onState.temp = 26;
    if (!remote.supportsSwingH) {
      onState.swingh = "off";
    }

    if (!sendCommand(buildAcCommand(remote.id, onState))) {
      return std::nullopt;
    }
    if (!confirmPowerResponse(remote)) {
      continue;
    }

    // 第二步：在开机状态基础上改一次温度。
    // 如果空调能响应温度变化，基本可以认为这个候选遥控器是可用的。
    AcState tempState = onState;
    tempState.temp = qMin(27, remote.maxTemp);
    if (!sendCommand(buildAcCommand(remote.id, tempState))) {
      return std::nullopt;
    }
    if (!confirmTempResponse(remote)) {
      continue;
    }

    bool named = false;
    const QString name = askDeviceName(remote, &named);
    if (!named) {
      return std::nullopt;
    }

    // 匹配成功后生成本地设备记录。保存到 JSON 的动作由 MainWindow 完成。
    KnownAcDevice device;
    device.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    device.name =
        name.trimmed().isEmpty() ? defaultDeviceName(remote) : name.trimmed();
    device.brandId = brand->id;
    device.remoteId = remote.id;
    device.state = onState;
    return device;
  }

  QMessageBox::information(m_parent, QStringLiteral("未匹配"),
                           QStringLiteral("该品牌候选遥控器已尝试完毕。"));
  return std::nullopt;
}

const AcBrand* AcDiscoveryWizard::selectBrand() const {
  // QInputDialog 只能返回字符串，所以这里用显示名反查品牌。
  // 目录较小，线性反查足够清楚。
  QStringList brandNames;
  for (const auto& brand : m_catalog) {
    brandNames << brandDisplayName(brand);
  }

  bool accepted = false;
  const QString selected = QInputDialog::getItem(
      m_parent, QStringLiteral("选择品牌"), QStringLiteral("品牌"), brandNames,
      0, false, &accepted);
  if (!accepted || selected.isEmpty()) {
    return nullptr;
  }

  for (const auto& brand : m_catalog) {
    if (brandDisplayName(brand) == selected) {
      return &brand;
    }
  }
  return nullptr;
}

bool AcDiscoveryWizard::confirmPowerResponse(const AcRemote& remote) const {
  // 物理空调是否响应无法从串口自动得知，所以这里必须让用户观察后确认。
  const auto reply = QMessageBox::question(
      m_parent, QStringLiteral("开机响应"),
      QStringLiteral("%1 是否响应开机？").arg(remote.name));
  return reply == QMessageBox::Yes;
}

bool AcDiscoveryWizard::confirmTempResponse(const AcRemote& remote) const {
  // 温度可调是比单纯开机更强的匹配条件，可以减少“开机码碰巧相同”的误判。
  const auto reply = QMessageBox::question(
      m_parent, QStringLiteral("温度响应"),
      QStringLiteral("%1 温度是否可调？").arg(remote.name));
  return reply == QMessageBox::Yes;
}

QString AcDiscoveryWizard::askDeviceName(const AcRemote& remote,
                                         bool* accepted) const {
  // accepted 用来区分“用户取消”和“用户输入空字符串”。
  // 空字符串仍允许保存，只是会退回默认名称。
  return QInputDialog::getText(m_parent, QStringLiteral("保存空调"),
                               QStringLiteral("名称"), QLineEdit::Normal,
                               defaultDeviceName(remote), accepted);
}
