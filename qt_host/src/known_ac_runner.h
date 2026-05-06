#pragma once

#include "ac_catalog.h"

#include <QObject>
#include <QTimer>

#include <functional>

class KnownAcRunner : public QObject {
    Q_OBJECT

public:
    using SendCommand = std::function<bool(const QString &command)>;

    explicit KnownAcRunner(QObject *parent = nullptr);

    bool isRunning() const;
    void start(QVector<KnownAcDevice> devices, const SendCommand &sendCommand);
    void stop();

signals:
    void finished();

private slots:
    void sendNext();

private:
    QVector<KnownAcDevice> m_devices;
    SendCommand m_sendCommand;
    QTimer m_timer;
    int m_index = 0;
};
