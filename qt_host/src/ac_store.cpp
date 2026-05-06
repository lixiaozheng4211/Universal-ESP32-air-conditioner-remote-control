#include "ac_store.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

QJsonObject stateToJson(const AcState &state)
{
    QJsonObject obj;
    obj["power"] = state.power;
    obj["mode"] = state.mode;
    obj["temp"] = state.temp;
    obj["fan"] = state.fan;
    obj["swingv"] = state.swingv;
    obj["swingh"] = state.swingh;
    obj["eco"] = state.eco;
    return obj;
}

AcState stateFromJson(const QJsonObject &obj)
{
    AcState state;
    state.power = obj.value("power").toBool(true);
    state.mode = obj.value("mode").toString("cool");
    state.temp = obj.value("temp").toInt(26);
    state.fan = obj.value("fan").toString("auto");
    state.swingv = obj.value("swingv").toString("off");
    state.swingh = obj.value("swingh").toString("off");
    state.eco = obj.value("eco").toBool(false);
    return state;
}

}

AcStore::AcStore()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::currentPath();
    }
    QDir().mkpath(dir);
    m_filePath = QDir(dir).filePath("discovered_ac.json");
}

QVector<KnownAcDevice> AcStore::load(QString *errorMessage) const
{
    QVector<KnownAcDevice> devices;
    QFile file(m_filePath);
    if (!file.exists()) {
        return devices;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return devices;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        if (errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return devices;
    }

    const QJsonArray array = doc.array();
    for (const auto &value : array) {
        const QJsonObject obj = value.toObject();
        KnownAcDevice device;
        device.id = obj.value("id").toString();
        device.name = obj.value("name").toString();
        device.brandId = obj.value("brandId").toString();
        device.remoteId = obj.value("remoteId").toString();
        device.state = stateFromJson(obj.value("state").toObject());
        if (!device.id.isEmpty() && !device.remoteId.isEmpty()) {
            devices.push_back(device);
        }
    }
    return devices;
}

bool AcStore::save(const QVector<KnownAcDevice> &devices, QString *errorMessage) const
{
    QJsonArray array;
    for (const auto &device : devices) {
        QJsonObject obj;
        obj["id"] = device.id;
        obj["name"] = device.name;
        obj["brandId"] = device.brandId;
        obj["remoteId"] = device.remoteId;
        obj["state"] = stateToJson(device.state);
        array.push_back(obj);
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

QString AcStore::filePath() const
{
    return m_filePath;
}
