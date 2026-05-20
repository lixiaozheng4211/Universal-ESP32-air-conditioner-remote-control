#pragma once

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>

// 串口控制器，把 QtSerialPort 封装成“按行收发文本协议”的接口。
// ESP32 固件端使用 \n 结束一条命令/响应，所以 UI 层不需要处理半包、粘包问题。
class SerialController : public QObject {
    Q_OBJECT

public:
    explicit SerialController(QObject *parent = nullptr);

    // 返回当前系统识别到的串口名，例如 COM3、COM5。
    QStringList availablePorts() const;

    // 打开 ESP32 串口。默认协议波特率固定为 115200。
    bool open(const QString &portName, QString *errorMessage = nullptr);
    void close();
    bool isOpen() const;

    // 发送一行命令，函数内部会自动追加 '\n'。
    bool sendLine(const QString &line, QString *errorMessage = nullptr);
    QString portName() const;

signals:
    // 每收到一条完整非空响应行就发出该信号，例如 OK PONG、ERR BAD_ACTION。
    void lineReceived(const QString &line);

    // 供主窗口状态栏显示“已连接 COMx / 未连接”。
    void statusChanged(const QString &status);

    // 串口底层报告设备丢失、读写错误等异常时发出。
    void connectionLost(const QString &reason);

private slots:
    // readyRead 信号入口。读取串口缓冲区并按换行拆包。
    void readReadyData();
    void handlePortError(QSerialPort::SerialPortError error);

private:
    QSerialPort m_port;

    // 保存尚未遇到换行符的半行数据。
    QByteArray m_buffer;
};
