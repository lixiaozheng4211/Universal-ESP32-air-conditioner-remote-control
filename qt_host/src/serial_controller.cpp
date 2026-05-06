#include "serial_controller.h"

SerialController::SerialController(QObject *parent)
    : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead, this, &SerialController::readReadyData);
}

QStringList SerialController::availablePorts() const
{
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
    if (m_port.isOpen()) {
        m_port.close();
    }

    m_port.setPortName(portName);
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

    QByteArray payload = line.toUtf8();
    payload.append('\n');
    const qint64 written = m_port.write(payload);
    if (written != payload.size()) {
        if (errorMessage) {
            *errorMessage = m_port.errorString();
        }
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
