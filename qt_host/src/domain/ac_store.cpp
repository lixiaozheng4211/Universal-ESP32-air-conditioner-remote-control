#include "ac_store.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

// JSON 中的 state 字段和 AcState 一一对应。
// 保存完整状态的好处是：下次打开详情页时能直接恢复上一次成功发送后的 UI。
QJsonObject stateToJson(const AcState& state) {
  QJsonObject obj;
  obj["power"] = state.power;
  obj["mode"] = state.mode;
  obj["temp"] = state.temp;
  obj["fan"] = state.fan;
  obj["swingv"] = state.swingv;
  obj["swingh"] = state.swingh;
  return obj;
}

AcState stateFromJson(const QJsonObject& obj) {
  // 每个字段都给默认值，旧版本 JSON 缺字段时仍能正常加载。
  AcState state;
  state.power = obj.value("power").toBool(true);
  state.mode = obj.value("mode").toString("cool");
  state.temp = obj.value("temp").toInt(26);
  state.fan = obj.value("fan").toString("auto");
  state.swingv = obj.value("swingv").toString("off");
  state.swingh = obj.value("swingh").toString("off");
  return state;
}

}  // namespace

AcStore::AcStore() {
  // 使用 AppDataLocation，而不是项目目录。
  // 这样无论从 IDE、build 目录还是双击 exe 启动，都会读写同一个用户配置位置。
  QString dir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (dir.isEmpty()) {
    dir = QDir::currentPath();
  }
  QDir().mkpath(dir);
  m_filePath = QDir(dir).filePath("discovered_ac.json");
}

QVector<KnownAcDevice> AcStore::load(QString* errorMessage) const {
  QVector<KnownAcDevice> devices;
  QFile file(m_filePath);
  if (!file.exists()) {
    // 第一次运行没有文件是正常情况，不当作错误。
    return devices;
  }
  if (!file.open(QIODevice::ReadOnly)) {
    if (errorMessage) {
      *errorMessage = file.errorString();
    }
    return devices;
  }

  QJsonParseError parseError;
  const QJsonDocument doc =
      QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
    if (errorMessage) {
      *errorMessage = parseError.errorString();
    }
    return devices;
  }

  const QJsonArray array = doc.array();
  for (const auto& value : array) {
    const QJsonObject obj = value.toObject();
    KnownAcDevice device;
    device.id = obj.value("id").toString();
    device.name = obj.value("name").toString();
    device.brandId = obj.value("brandId").toString();
    device.remoteId = obj.value("remoteId").toString();
    device.state = stateFromJson(obj.value("state").toObject());
    // id 和 remoteId 是最小可用条件。显示名或品牌缺失时 UI 仍可以降级显示。
    if (!device.id.isEmpty() && !device.remoteId.isEmpty()) {
      devices.push_back(device);
    }
  }
  return devices;
}

bool AcStore::save(const QVector<KnownAcDevice>& devices,
                   QString* errorMessage) const {
  // 目前数据量很小，直接全量覆盖比增量更新更简单，也能避免删除后残留旧记录。
  QJsonArray array;
  for (const auto& device : devices) {
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

QString AcStore::filePath() const { return m_filePath; }
