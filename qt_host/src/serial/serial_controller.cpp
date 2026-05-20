#include "serial_controller.h"

SerialController::SerialController(QObject *parent)
    : QObject(parent)
{
    // QtSerialPort 是异步接口；readyRead 到来时只说明“有新字节”，不保证刚好是一整行。
    connect(&m_port, &QSerialPort::readyRead, this, &SerialController::readReadyData);
    connect(&m_port, &QSerialPort::errorOccurred, this, &SerialController::handlePortError);
}

QStringList SerialController::availablePorts() const
{
    // 只展示 portName，Windows 下就是 COMx。描述、VID/PID 暂时不放到 UI，保持选择框简洁。
    QStringList result;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &port : ports) {
        result.push_back(port.portName());
    }
    result.sort();
    return result;
}

bool SerialController::open(const QString &portName, QString *errorMessage)
{
    // 切换串口时先关闭旧连接，避免同一个 QSerialPort 保留旧端口状态。
    if (m_port.isOpen()) {
        m_port.close();
    }

    m_port.setPortName(portName);
    // 固件协议约定 115200 8N1，无硬件流控。
    m_port.setBaudRate(115200);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadWrite)) {
        if (errorMessage) {
            *errorMessage = m_port.errorString();
        }
        emit statusChanged(QStringLiteral("未连接"));
        return false;
    }

    m_buffer.clear();
    emit statusChanged(QStringLiteral("已连接 %1").arg(portName));
    return true;
}

void SerialController::close()
{
    if (m_port.isOpen()) {
        m_port.close();
    }
    m_buffer.clear();
    emit statusChanged(QStringLiteral("未连接"));
}

bool SerialController::isOpen() const
{
    return m_port.isOpen();
}

bool SerialController::sendLine(const QString &line, QString *errorMessage)
{
    if (!m_port.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("串口未连接");
        }
        return false;
    }

    // 固件按 '\n' 作为命令结束符；统一在这里追加，调用方只传命令正文。
    QByteArray payload = line.toUtf8();
    payload.append('\n');
    const qint64 written = m_port.write(payload);
    if (written != payload.size()) {
        if (errorMessage) {
            *errorMessage = m_port.errorString();
        }
        emit connectionLost(m_port.errorString());
        return false;
    }
    return true;
}

QString SerialController::portName() const
{
    return m_port.portName();
}

void SerialController::readReadyData()
{
    m_buffer += m_port.readAll();
    while (true) {
        // 串口可能一次收到半行，也可能一次收到多行；循环拆出所有完整行。
        const int newline = m_buffer.indexOf('\n');
        if (newline < 0) {
            break;
        }
        QByteArray line = m_buffer.left(newline);
        m_buffer.remove(0, newline + 1);
        line = line.trimmed();
        if (!line.isEmpty()) {
            emit lineReceived(QString::fromUtf8(line));
        }
    }
}

void SerialController::handlePortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::NotOpenError) {
        return;
    }
    if (!m_port.isOpen()) {
        return;
    }

    const QString reason = m_port.errorString();
    m_port.close();
    m_buffer.clear();
    emit statusChanged(QStringLiteral("未连接"));
    emit connectionLost(reason);
}
