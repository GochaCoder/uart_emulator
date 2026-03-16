#include "emulateddevice.h"
#include <QDebug>
#include <QRandomGenerator>

EmulatedDevice::EmulatedDevice(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_periodicTimer(new QTimer(this))
    , m_ledState(false)
    , m_temperature(24.5)
    , m_emulationEnabled(false)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &EmulatedDevice::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &EmulatedDevice::handleError);
    connect(m_periodicTimer, &QTimer::timeout, this, &EmulatedDevice::sendPeriodicData);
    m_periodicTimer->start(2000);
}

EmulatedDevice::~EmulatedDevice()
{
    closePort();
}

bool EmulatedDevice::openPort(const QString &portName, qint32 baudRate)
{
    if (m_serialPort->isOpen())
        m_serialPort->close();

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("Не удалось открыть порт %1: %2")
                               .arg(portName, m_serialPort->errorString()));
        return false;
    }

    qDebug() << "Порт" << portName << "открыт";
    return true;
}

void EmulatedDevice::closePort()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        qDebug() << "Порт закрыт";
    }
}

void EmulatedDevice::setEmulationEnabled(bool enabled)
{
    m_emulationEnabled = enabled;
}

void EmulatedDevice::sendData(const QByteArray &data)
{
    if (m_serialPort->isOpen()) {
        m_serialPort->write(data);
        m_serialPort->flush();
        emit dataSent(data);
    }
}

void EmulatedDevice::handleReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    emit dataReceived(data);

    if (m_emulationEnabled) {
        m_buffer.append(data);
        while (m_buffer.contains('\n')) {
            int pos = m_buffer.indexOf('\n');
            QByteArray line = m_buffer.left(pos).trimmed();
            m_buffer.remove(0, pos + 1);
            if (!line.isEmpty()) {
                qDebug() << "Получена команда:" << line;
                processCommand(line);
            }
        }
    }
}

void EmulatedDevice::processCommand(const QByteArray &cmd)
{
    QByteArray response;

    if (cmd == "led on") {
        m_ledState = true;
        response = "LED ON OK\r\n";
    }
    else if (cmd == "led off") {
        m_ledState = false;
        response = "LED OFF OK\r\n";
    }
    else if (cmd == "led?") {
        response = m_ledState ? "LED is ON\r\n" : "LED is OFF\r\n";
    }
    else if (cmd == "temp?") {
        m_temperature += (QRandomGenerator::global()->generateDouble() - 0.5) * 0.2;
        response = QString("Temperature: %1 C\r\n").arg(m_temperature, 0, 'f', 1).toUtf8();
    }
    else if (cmd == "status") {
        response = QString("LED: %1, Temp: %2 C\r\n")
        .arg(m_ledState ? "ON" : "OFF")
            .arg(m_temperature, 0, 'f', 1).toUtf8();
    }
    else {
        response = "Unknown command\r\n";
    }

    // Отправляем ответ через порт (он попадёт в dataSent)
    sendData(response);
}

void EmulatedDevice::sendPeriodicData()
{
    if (m_emulationEnabled && m_serialPort->isOpen()) {
        QByteArray data = QString("Periodic: temp=%1 C\r\n").arg(m_temperature, 0, 'f', 1).toUtf8();
        sendData(data);
        qDebug() << "Периодическое сообщение отправлено";
    }
}

void EmulatedDevice::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        emit errorOccurred(m_serialPort->errorString());
    }
}
