#pragma once

#include "ac_catalog.h"

#include <QWidget>

#include <functional>
#include <optional>

class AcDiscoveryWizard {
public:
    using SendCommand = std::function<bool(const QString &command)>;

    AcDiscoveryWizard(QWidget *parent, const QVector<AcBrand> &catalog);

    std::optional<KnownAcDevice> run(const SendCommand &sendCommand);

private:
    const AcBrand *selectBrand() const;
    bool confirmPowerResponse(const AcRemote &remote) const;
    bool confirmTempResponse(const AcRemote &remote) const;
    QString askDeviceName(const AcRemote &remote, bool *accepted) const;

    QWidget *m_parent = nullptr;
    const QVector<AcBrand> &m_catalog;
};
