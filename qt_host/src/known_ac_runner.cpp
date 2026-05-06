#include "known_ac_runner.h"

KnownAcRunner::KnownAcRunner(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(1500);
    connect(&m_timer, &QTimer::timeout, this, &KnownAcRunner::sendNext);
}

bool KnownAcRunner::isRunning() const
{
    return m_timer.isActive();
}

void KnownAcRunner::start(QVector<KnownAcDevice> devices, const SendCommand &sendCommand)
{
    stop();
    m_devices = std::move(devices);
    m_sendCommand = sendCommand;
    m_index = 0;

    sendNext();
    if (!m_devices.isEmpty()) {
        m_timer.start();
    }
}

void KnownAcRunner::stop()
{
    m_timer.stop();
    m_devices.clear();
    m_sendCommand = {};
    m_index = 0;
}

void KnownAcRunner::sendNext()
{
    if (m_index >= m_devices.size()) {
        stop();
        emit finished();
        return;
    }

    KnownAcDevice device = m_devices[m_index++];
    device.state.power = true;
    if (m_sendCommand && !m_sendCommand(buildAcCommand(device.remoteId, device.state))) {
        stop();
        emit finished();
    }
}
