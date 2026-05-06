#pragma once

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>

class SerialController : public QObject {
    Q_OBJECT

public:
    explicit SerialController(QObject *parent = nullptr);

    QStringList availablePorts() const;
    bool open(const QString &portName, QString *errorMessage = nullptr);
    void close();
    bool isOpen() const;
    bool sendLine(const QString &line, QString *errorMessage = nullptr);
    QString portName() const;

signals:
    void lineReceived(const QString &line);
    void statusChanged(const QString &status);

private slots:
    void readReadyData();

private:
    QSerialPort m_port;
    QByteArray m_buffer;
};
