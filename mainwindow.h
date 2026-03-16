#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPortInfo>
#include "emulateddevice.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenCloseButtonClicked();
    void onSendButtonClicked();
    void onEmulationToggled(bool checked);
    void appendData(const QByteArray &data, bool incoming);
    void showError(const QString &error);
    void refreshPortsList();                  // новый слот для кнопки обновления

private:
    void fillPortsInfo();                     // заполняет комбобокс портами

    EmulatedDevice *m_device;
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QPushButton *m_openCloseButton;
    QPushButton *m_refreshButton;              // кнопка обновления списка
    QTextEdit *m_logEdit;
    QLineEdit *m_commandEdit;
    QPushButton *m_sendButton;
    QCheckBox *m_emulationCheckBox;
    bool m_portOpen;
};

#endif // MAINWINDOW_H
