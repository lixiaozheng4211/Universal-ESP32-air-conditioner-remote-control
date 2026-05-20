#pragma once

#include "ac_catalog.h"

#include <QString>
#include <QVector>

// 本机空调库的读写封装。
// 第一版只把已匹配的空调保存在电脑端 discovered_ac.json，不写入 ESP32。
// 这样换 ESP32 固件不会丢配置，调试时也更容易直接检查 JSON 内容。
class AcStore {
public:
    AcStore();

    // 读取所有已保存空调。出错时返回能成功解析的空列表，并通过 errorMessage 给 UI 提示。
    QVector<KnownAcDevice> load(QString *errorMessage = nullptr) const;

    // 覆盖写入完整设备列表。删除、添加、详情控制成功后都会调用它。
    bool save(const QVector<KnownAcDevice> &devices, QString *errorMessage = nullptr) const;

    // 暴露路径主要方便调试时定位 discovered_ac.json。
    QString filePath() const;

private:
    // Qt 标准应用数据目录下的 discovered_ac.json。
    QString m_filePath;
};
