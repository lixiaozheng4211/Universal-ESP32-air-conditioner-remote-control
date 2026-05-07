#pragma once

#include <QObject>
#include <QTimer>
#include <functional>

#include "ac_catalog.h"

// “开启空调”批量执行器。
// 它把已保存空调按 1500ms 间隔逐个开机，避免连续红外发射过密，也避免 UI 阻塞。
// MainWindow 只需要提供 SendCommand，真正的定时节奏由这个类维护。
class KnownAcRunner : public QObject {
  Q_OBJECT

 public:
  // 参数是已经拼好的串口命令，例如 AC remote=... action=power power=1。
  using SendCommand = std::function<bool(const QString& command)>;

  explicit KnownAcRunner(QObject* parent = nullptr);

  bool isRunning() const;

  // 传入设备列表副本，运行期间即使主窗口表格刷新，也不会影响当前批量开机顺序。
  void start(QVector<KnownAcDevice> devices, const SendCommand& sendCommand);
  void stop();

 signals:
  void finished();

 private slots:
  // 定时器每次触发只发送一个设备，直到列表全部处理完。
  void sendNext();

 private:
  QVector<KnownAcDevice> m_devices;
  SendCommand m_sendCommand;
  QTimer m_timer;
  int m_index = 0;
};
