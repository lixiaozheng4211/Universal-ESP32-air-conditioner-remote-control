#include "known_ac_runner.h"

KnownAcRunner::KnownAcRunner(QObject* parent) : QObject(parent) {
  // 批量开关机只发送 action=power，Qt 侧按 50ms 逐台写入串口。
  // ESP32 固件会同步执行红外发送，串口缓冲负责承接短时间内到达的命令。
  m_timer.setInterval(50);
  connect(&m_timer, &QTimer::timeout, this, &KnownAcRunner::sendNext);
}

bool KnownAcRunner::isRunning() const { return m_timer.isActive(); }

void KnownAcRunner::start(QVector<KnownAcDevice> devices, bool targetPower,
                          const SendCommand& sendCommand,
                          const DeviceSent& deviceSent) {
  // start 可以重复调用；先 stop 清空旧任务，保证一次只跑一组批量任务。
  stop();
  m_devices = std::move(devices);
  m_targetPower = targetPower;
  m_sendCommand = sendCommand;
  m_deviceSent = deviceSent;
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
  m_deviceSent = {};
  m_index = 0;
}

void KnownAcRunner::sendNext() {
  if (m_index >= m_devices.size()) {
    stop();
    emit finished();
    return;
  }

  const int currentIndex = m_index++;
  KnownAcDevice device = m_devices[currentIndex];
  device.state.power = m_targetPower;
  // 批量开关机只需要 action=power。对 RN02S13 这种遥控器，这能避免完整状态触发多条红外码。
  if (m_sendCommand &&
      !m_sendCommand(buildAcActionCommand(
          device.remoteId, QStringLiteral("power"), device.state))) {
    stop();
    emit finished();
    return;
  }

  if (m_deviceSent) {
    m_deviceSent(currentIndex, device.state);
  }
}
