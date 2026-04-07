#include "emulateddevice.h"
#include <QDebug>
#include <QRandomGenerator>
#include <QTime>


int update_timer_ms = 10;


static int randomBetween(int low, int high)
{
    return low + (rand() % (high - low + 1));
}

EmulatedDevice::EmulatedDevice(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_periodicTimer(new QTimer(this))
    , m_emulationEnabled(false)
    , m_angle(0.0)
    , m_rt(false)
    , m_speed(1)
    , m_emitt(false)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &EmulatedDevice::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &EmulatedDevice::handleError);
    connect(m_periodicTimer, &QTimer::timeout, this, &EmulatedDevice::sendPeriodicData);
    m_periodicTimer->start(update_timer_ms); // 10 ms
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

    if (cmd == "start") {
        response = "Start rotating\r\n";
        m_rt = true;
    }
    else if (cmd == "stop") {
        response = "Stop rotating\r\n";
        m_rt = false;
    }
    else if (cmd == "3") {
        response = "Speed set 3\r\n";
        m_speed = 3;
    }
    else if (cmd == "6") {
        response = "Speed set 6\r\n";
        m_speed = 6;
    }
    else if (cmd == "12") {
        response = "Speed set 12\r\n";
        m_speed = 12;
    }
    else if (cmd == "emit_on") {
        response = "Start emitting\r\n";
        m_emitt = true;
    }
    else if (cmd == "emit_off") {
        response = "Stop emitting\r\n";
        m_emitt = false;
    }
    else if (cmd == "zero") {
        response = "set zero\r\n";
        m_angle = 0.0;
    }
    else {
        response = "Unknown command\r\n";
    }

    sendData(response);
}

void EmulatedDevice::sendPeriodicData()
{
    if (m_emulationEnabled && m_serialPort->isOpen()) {
        if (m_rt) {
            double step = (m_speed * 6) * (update_timer_ms / 1000.0);
            m_angle += step;
            if (m_angle > 360) m_angle -= 360;
            //qDebug() << "Угол=" << m_angle << "Скорость=" << m_speed << "Шаг=" << step;
        }

        int dist = m_emitt ? 0 : -1;
        if (m_emitt) {
            // Ищем первую зону, в которую попадает текущий угол
            for (const DetectionZone &zone : m_zones) {
                if (m_angle >= zone.minAngle && m_angle <= zone.maxAngle) {
                    dist = randomBetween(zone.minDist, zone.maxDist);
                    break;
                }
            }
        }

        QByteArray data = QString("111 %1 %2 00000\n").arg(m_angle, 0, 'f', 1).arg(dist).toUtf8();
        sendData(data);
        //qDebug() << "Периодическое сообщение отправлено";
    }
}

void EmulatedDevice::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        emit errorOccurred(m_serialPort->errorString());
    }
}

// Управление зонами
void EmulatedDevice::setZones(const QVector<DetectionZone> &zones)
{
    m_zones = zones;
}

void EmulatedDevice::addZone(const DetectionZone &zone)
{
    m_zones.append(zone);
}

void EmulatedDevice::removeZone(int index)
{
    if (index >= 0 && index < m_zones.size())
        m_zones.remove(index);
}

void EmulatedDevice::clearZones()
{
    m_zones.clear();
}
