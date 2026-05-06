#pragma once

#include "ac_catalog.h"

#include <QString>
#include <QVector>

class AcStore {
public:
    AcStore();

    QVector<KnownAcDevice> load(QString *errorMessage = nullptr) const;
    bool save(const QVector<KnownAcDevice> &devices, QString *errorMessage = nullptr) const;
    QString filePath() const;

private:
    QString m_filePath;
};
