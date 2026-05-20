#pragma once

#include <QObject>
#include <QTimer>
#include <functional>

#include "ac_catalog.h"

// 批量空调电源执行器。
// 它把已保存空调按 50ms 间隔逐个写入串口开/关，也避免 UI 阻塞。
// MainWindow 只需要提供 SendCommand，真正的定时节奏由这个类维护。
class KnownAcRunner : public QObject {
  Q_OBJECT

 public:
  // 参数是已经拼好的串口命令，例如 AC remote=... action=power power=1。
  using SendCommand = std::function<bool(const QString& command)>;
  using DeviceSent = std::function<void(int index, const AcState& state)>;

  explicit KnownAcRunner(QObject* parent = nullptr);

  bool isRunning() const;

  // 传入设备列表副本，运行期间即使主窗口刷新，也不会影响当前批量顺序。
  void start(QVector<KnownAcDevice> devices, bool targetPower,
             const SendCommand& sendCommand, const DeviceSent& deviceSent);
  void stop();

 signals:
  void finished();

 private slots:
  // 定时器每次触发只发送一个设备，直到列表全部处理完。
  void sendNext();

 private:
  QVector<KnownAcDevice> m_devices;
  SendCommand m_sendCommand;
  DeviceSent m_deviceSent;
  QTimer m_timer;
  bool m_targetPower = true;
  int m_index = 0;
};
