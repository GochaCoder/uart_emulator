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
class QTableWidget;
class QTableWidgetItem;
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
    void refreshPortsList();
    void onAddZoneClicked();
    void onRemoveZoneClicked();
    void onApplyZonesClicked();
    void onLoadExampleZonesClicked();
    void onClearAllZonesClicked();

private:
    void fillPortsInfo();
    void updateZoneTable();  // обновить таблицу из устройства (при необходимости)

    EmulatedDevice *m_device;
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QPushButton *m_openCloseButton;
    QPushButton *m_refreshButton;
    QTextEdit *m_logEdit;
    QLineEdit *m_commandEdit;
    QPushButton *m_sendButton;
    QCheckBox *m_emulationCheckBox;
    QPushButton *m_loadExampleButton;
    QPushButton *m_clearAllButton;
    bool m_portOpen;

    // Таблица для зон
    QTableWidget *m_zoneTable;
    QPushButton *m_addZoneButton;
    QPushButton *m_removeZoneButton;
    QPushButton *m_applyZonesButton;
};

#endif // MAINWINDOW_H
