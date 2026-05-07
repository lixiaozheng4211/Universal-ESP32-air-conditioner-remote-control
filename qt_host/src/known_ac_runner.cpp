#include "known_ac_runner.h"

KnownAcRunner::KnownAcRunner(QObject* parent) : QObject(parent) {
  // 1500ms 是经验间隔：给红外接收和空调蜂鸣响应留出时间，也防止 ESP32 连续发射过密。
  m_timer.setInterval(1500);
  connect(&m_timer, &QTimer::timeout, this, &KnownAcRunner::sendNext);
}

bool KnownAcRunner::isRunning() const { return m_timer.isActive(); }

void KnownAcRunner::start(QVector<KnownAcDevice> devices,
                          const SendCommand& sendCommand) {
  // start 可以重复调用；先 stop 清空旧任务，保证一次只跑一组批量开机。
  stop();
  m_devices = std::move(devices);
  m_sendCommand = sendCommand;
  m_index = 0;

  sendNext();
  if (!m_devices.isEmpty()) {
    // 第一台立即发送，后续设备按定时器间隔发送，用户不用等待首条命令。
    m_timer.start();
  }
}

void KnownAcRunner::stop() {
  m_timer.stop();
  m_devices.clear();
  m_sendCommand = {};
  m_index = 0;
}

void KnownAcRunner::sendNext() {
  if (m_index >= m_devices.size()) {
    stop();
    emit finished();
    return;
  }

  KnownAcDevice device = m_devices[m_index++];
  device.state.power = true;
  // 批量开机只需要 action=power。对 RN02S13 这种遥控器，这能避免完整状态触发多条红外码。
  if (m_sendCommand &&
      !m_sendCommand(buildAcActionCommand(
          device.remoteId, QStringLiteral("power"), device.state))) {
    stop();
    emit finished();
  }
}
