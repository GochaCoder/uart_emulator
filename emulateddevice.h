#ifndef EMULATEDDEVICE_H
#define EMULATEDDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>

class EmulatedDevice : public QObject
{
    Q_OBJECT
public:
    explicit EmulatedDevice(QObject *parent = nullptr);
    ~EmulatedDevice();

    bool openPort(const QString &portName, qint32 baudRate = QSerialPort::Baud115200);
    void closePort();
    void setEmulationEnabled(bool enabled);   // вкл/выкл обработку команд
    void sendData(const QByteArray &data);    // отправка данных в порт

signals:
    void errorOccurred(const QString &error);
    void dataReceived(const QByteArray &data); // сигнал для отображения входящих данных
    void dataSent(const QByteArray &data);     // сигнал для отображения исходящих данных

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void sendPeriodicData();

private:
    void processCommand(const QByteArray &cmd);

    QSerialPort *m_serialPort;
    QTimer *m_periodicTimer;
    QByteArray m_buffer;
    bool m_ledState;
    double m_temperature;
    bool m_emulationEnabled;                   // флаг режима эмуляции
};

#endif // EMULATEDDEVICE_H
