#include "ac_discovery_wizard.h"

#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QUuid>

AcDiscoveryWizard::AcDiscoveryWizard(QWidget *parent, const QVector<AcBrand> &catalog)
    : m_parent(parent)
    , m_catalog(catalog)
{
}

std::optional<KnownAcDevice> AcDiscoveryWizard::run(const SendCommand &sendCommand)
{
    const AcBrand *brand = selectBrand();
    if (!brand) {
        return std::nullopt;
    }

    for (const auto &remote : brand->remotes) {
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

        KnownAcDevice device;
        device.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        device.name = name.trimmed().isEmpty() ? defaultDeviceName(remote) : name.trimmed();
        device.brandId = brand->id;
        device.remoteId = remote.id;
        device.state = onState;
        return device;
    }

    QMessageBox::information(m_parent, QStringLiteral("未匹配"), QStringLiteral("该品牌候选遥控器已尝试完毕。"));
    return std::nullopt;
}

const AcBrand *AcDiscoveryWizard::selectBrand() const
{
    QStringList brandNames;
    for (const auto &brand : m_catalog) {
        brandNames << brandDisplayName(brand);
    }

    bool accepted = false;
    const QString selected = QInputDialog::getItem(m_parent, QStringLiteral("选择品牌"), QStringLiteral("品牌"), brandNames, 0, false, &accepted);
    if (!accepted || selected.isEmpty()) {
        return nullptr;
    }

    for (const auto &brand : m_catalog) {
        if (brandDisplayName(brand) == selected) {
            return &brand;
        }
    }
    return nullptr;
}

bool AcDiscoveryWizard::confirmPowerResponse(const AcRemote &remote) const
{
    const auto reply = QMessageBox::question(m_parent, QStringLiteral("开机响应"), QStringLiteral("%1 是否响应开机？").arg(remote.name));
    return reply == QMessageBox::Yes;
}

bool AcDiscoveryWizard::confirmTempResponse(const AcRemote &remote) const
{
    const auto reply = QMessageBox::question(m_parent, QStringLiteral("温度响应"), QStringLiteral("%1 温度是否可调？").arg(remote.name));
    return reply == QMessageBox::Yes;
}

QString AcDiscoveryWizard::askDeviceName(const AcRemote &remote, bool *accepted) const
{
    return QInputDialog::getText(m_parent, QStringLiteral("保存空调"), QStringLiteral("名称"), QLineEdit::Normal, defaultDeviceName(remote), accepted);
}
