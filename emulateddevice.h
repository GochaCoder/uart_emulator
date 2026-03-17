#ifndef EMULATEDDEVICE_H
#define EMULATEDDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QVector>

// Структура, описывающая одну зону обнаружения
struct DetectionZone {
    double minAngle;
    double maxAngle;
    int minDist;
    int maxDist;

    DetectionZone(double minAng = 0, double maxAng = 0, int minD = 0, int maxD = 0)
        : minAngle(minAng), maxAngle(maxAng), minDist(minD), maxDist(maxD) {}
};

class EmulatedDevice : public QObject
{
    Q_OBJECT
public:
    explicit EmulatedDevice(QObject *parent = nullptr);
    ~EmulatedDevice();

    bool openPort(const QString &portName, qint32 baudRate = QSerialPort::Baud115200);
    void closePort();
    void setEmulationEnabled(bool enabled);
    void sendData(const QByteArray &data);

    // Управление зонами обнаружения
    void setZones(const QVector<DetectionZone> &zones);
    void addZone(const DetectionZone &zone);
    void removeZone(int index);
    void clearZones();

signals:
    void errorOccurred(const QString &error);
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void sendPeriodicData();

private:
    void processCommand(const QByteArray &cmd);

    QSerialPort *m_serialPort;
    QTimer *m_periodicTimer;
    QByteArray m_buffer;
    bool m_emulationEnabled;

    // Состояние эмуляции
    double m_angle;
    bool m_rt;              // вращение включено/выключено
    int m_speed;            // скорость вращения (градусов/сек?)
    bool m_emitt;           // разрешена ли генерация дальности

    // Список зон обнаружения
    QVector<DetectionZone> m_zones;
};

#endif // EMULATEDDEVICE_H
