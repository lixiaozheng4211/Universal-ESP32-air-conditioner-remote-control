#pragma once

#include <QString>
#include <QVector>

struct AcState {
    bool power = true;
    QString mode = "cool";
    int temp = 26;
    QString fan = "auto";
    QString swingv = "off";
    QString swingh = "off";
    bool eco = false;
};

struct AcRemote {
    QString id;
    QString brandId;
    QString name;
    int minTemp = 17;
    int maxTemp = 30;
    bool supportsEco = false;
    bool supportsSwingV = false;
    bool supportsSwingH = false;
};

struct AcBrand {
    QString id;
    QString name;
    QVector<AcRemote> remotes;
};

struct KnownAcDevice {
    QString id;
    QString name;
    QString brandId;
    QString remoteId;
    AcState state;
};

QVector<AcBrand> defaultAcCatalog();
const AcRemote *findRemote(const QVector<AcBrand> &catalog, const QString &remoteId);
QString buildAcCommand(const QString &remoteId, const AcState &state);
QString brandDisplayName(const AcBrand &brand);
QString defaultDeviceName(const AcRemote &remote);
