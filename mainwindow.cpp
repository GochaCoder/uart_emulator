#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QScrollBar>
#include <QComboBox>
#include <QSerialPortInfo>
#include <QPushButton>
#include <QWidget>
#include <QCheckBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QDoubleSpinBox>
#include <QSpinBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_device(new EmulatedDevice(this))
    , m_portOpen(false)
{
    setWindowTitle("Эмулятор UART / Работы УМРЛС (множественные зоны)");

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Группа настроек порта (без изменений)
    QGroupBox *portGroup = new QGroupBox("Настройки порта");
    QHBoxLayout *portLayout = new QHBoxLayout(portGroup);
    portLayout->addWidget(new QLabel("Порт:"));
    m_portCombo = new QComboBox;
    fillPortsInfo();
    portLayout->addWidget(m_portCombo);
    m_refreshButton = new QPushButton("Обновить");
    portLayout->addWidget(m_refreshButton);
    portLayout->addWidget(new QLabel("Скорость:"));
    m_baudCombo = new QComboBox;
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "230400"});
    m_baudCombo->setCurrentText("115200");
    portLayout->addWidget(m_baudCombo);
    m_openCloseButton = new QPushButton("Открыть порт");
    portLayout->addWidget(m_openCloseButton);
    m_emulationCheckBox = new QCheckBox("Режим эмуляции работы УМРЛС");
    m_emulationCheckBox->setChecked(false);
    portLayout->addWidget(m_emulationCheckBox);
    mainLayout->addWidget(portGroup);

    // Группа для редактирования зон обнаружения
    QGroupBox *zonesGroup = new QGroupBox("Список целей");
    QVBoxLayout *zonesLayout = new QVBoxLayout(zonesGroup);

    m_zoneTable = new QTableWidget(0, 4);
    m_zoneTable->setHorizontalHeaderLabels({"Начальный угол", "Конечный угол", "Мин дальность", "Макс дальность"});
    m_zoneTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    zonesLayout->addWidget(m_zoneTable);

    QHBoxLayout *zoneButtonsLayout = new QHBoxLayout;
    m_addZoneButton = new QPushButton("Добавить цель");
    m_removeZoneButton = new QPushButton("Удалить цель");
    m_applyZonesButton = new QPushButton("Применить цели");
    m_loadExampleButton = new QPushButton("Загрузить примеры");
    m_clearAllButton = new QPushButton("Удалить все");

    connect(m_clearAllButton, &QPushButton::clicked, this, &MainWindow::onClearAllZonesClicked);
    connect(m_loadExampleButton, &QPushButton::clicked, this, &MainWindow::onLoadExampleZonesClicked);
    zoneButtonsLayout->addWidget(m_addZoneButton);
    zoneButtonsLayout->addWidget(m_removeZoneButton);
    zoneButtonsLayout->addWidget(m_loadExampleButton);
    zoneButtonsLayout->addStretch();
    zoneButtonsLayout->addWidget(m_clearAllButton);
    zoneButtonsLayout->addWidget(m_applyZonesButton);
    zonesLayout->addLayout(zoneButtonsLayout);

    mainLayout->addWidget(zonesGroup);

    // Лог сообщений
    QGroupBox *logGroup = new QGroupBox("Лог");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    m_logEdit = new QTextEdit;
    m_logEdit->setReadOnly(true);
    m_logEdit->setFontFamily("Courier New");
    logLayout->addWidget(m_logEdit);
    mainLayout->addWidget(logGroup);

    // Отправка команд
    QGroupBox *sendGroup = new QGroupBox("Отправка данных");
    QHBoxLayout *sendLayout = new QHBoxLayout(sendGroup);
    m_commandEdit = new QLineEdit;
    m_commandEdit->setPlaceholderText("Введите команду...");
    sendLayout->addWidget(m_commandEdit);
    m_sendButton = new QPushButton("Отправить");
    m_sendButton->setEnabled(false);
    sendLayout->addWidget(m_sendButton);
    mainLayout->addWidget(sendGroup);

    // Подключения сигналов
    connect(m_openCloseButton, &QPushButton::clicked, this, &MainWindow::onOpenCloseButtonClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(m_commandEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendButtonClicked);
    connect(m_emulationCheckBox, &QCheckBox::toggled, this, &MainWindow::onEmulationToggled);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPortsList);

    // Сигналы для работы с зонами
    connect(m_addZoneButton, &QPushButton::clicked, this, &MainWindow::onAddZoneClicked);
    connect(m_removeZoneButton, &QPushButton::clicked, this, &MainWindow::onRemoveZoneClicked);
    connect(m_applyZonesButton, &QPushButton::clicked, this, &MainWindow::onApplyZonesClicked);

    connect(m_device, &EmulatedDevice::dataReceived, this, [this](const QByteArray &data) {
        appendData(data, true);
    });
    connect(m_device, &EmulatedDevice::dataSent, this, [this](const QByteArray &data) {
        appendData(data, false);
    });
    connect(m_device, &EmulatedDevice::errorOccurred, this, &MainWindow::showError);
}

MainWindow::~MainWindow()
{
    m_device->closePort();
}

void MainWindow::fillPortsInfo()
{
    m_portCombo->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        m_portCombo->addItem(info.portName());
    }
    if (m_portCombo->count() == 0) {
        m_portCombo->addItem("Нет доступных портов");
    }
}

void MainWindow::refreshPortsList()
{
    QString currentPort = m_portCombo->currentText();
    fillPortsInfo();
    int index = m_portCombo->findText(currentPort);
    if (index != -1) {
        m_portCombo->setCurrentIndex(index);
    }
}

void MainWindow::onOpenCloseButtonClicked()
{
    if (!m_portOpen) {
        QString portName = m_portCombo->currentText();
        if (portName.isEmpty() || portName == "Нет доступных портов") {
            QMessageBox::warning(this, "Ошибка", "Выберите корректный порт");
            return;
        }
        qint32 baud = m_baudCombo->currentText().toInt();
        if (m_device->openPort(portName, baud)) {
            m_portOpen = true;
            m_openCloseButton->setText("Закрыть порт");
            m_sendButton->setEnabled(true);
            m_portCombo->setEnabled(false);
            m_baudCombo->setEnabled(false);
            m_refreshButton->setEnabled(false);
            appendData(tr(">>> Порт %1 открыт, скорость %2\n").arg(portName).arg(baud).toUtf8(), false);
        }
    } else {
        m_device->closePort();
        m_portOpen = false;
        m_openCloseButton->setText("Открыть порт");
        m_sendButton->setEnabled(false);
        m_portCombo->setEnabled(true);
        m_baudCombo->setEnabled(true);
        m_refreshButton->setEnabled(true);
        appendData(">>> Порт закрыт\n", false);
    }
}

void MainWindow::onSendButtonClicked()
{
    QString text = m_commandEdit->text();
    if (text.isEmpty()) return;
    m_commandEdit->clear();
    if (!text.endsWith('\n'))
        text += '\n';
    m_device->sendData(text.toUtf8());
}

void MainWindow::onEmulationToggled(bool checked)
{
    m_device->setEmulationEnabled(checked);
    appendData(tr(">>> Режим эмуляции %1\n").arg(checked ? "включен" : "отключен").toUtf8(), false);
}

void MainWindow::appendData(const QByteArray &data, bool incoming)
{
    QString prefix = incoming ? ">> " : "<< ";
    m_logEdit->moveCursor(QTextCursor::End);
    m_logEdit->insertPlainText(prefix + QString::fromUtf8(data));
    QScrollBar *vbar = m_logEdit->verticalScrollBar();
    vbar->setValue(vbar->maximum());
}

void MainWindow::showError(const QString &error)
{
    QMessageBox::critical(this, "Ошибка последовательного порта", error);
    if (m_portOpen) {
        m_portOpen = false;
        m_openCloseButton->setText("Открыть порт");
        m_sendButton->setEnabled(false);
        m_portCombo->setEnabled(true);
        m_baudCombo->setEnabled(true);
        m_refreshButton->setEnabled(true);
    }
}

// Работа с таблицей зон
void MainWindow::onAddZoneClicked()
{
    int row = m_zoneTable->rowCount();
    m_zoneTable->insertRow(row);
    // Устанавливаем делегаты для удобного ввода чисел с плавающей точкой и целых
    // Можно оставить простые ячейки, пользователь будет вводить числа.
    // Для удобства установим значения по умолчанию
    m_zoneTable->setItem(row, 0, new QTableWidgetItem("0.0"));
    m_zoneTable->setItem(row, 1, new QTableWidgetItem("0.0"));
    m_zoneTable->setItem(row, 2, new QTableWidgetItem("0"));
    m_zoneTable->setItem(row, 3, new QTableWidgetItem("0"));
}

void MainWindow::onRemoveZoneClicked()
{
    int currentRow = m_zoneTable->currentRow();
    if (currentRow >= 0) {
        m_zoneTable->removeRow(currentRow);
    } else {
        QMessageBox::information(this, "Удаление", "Выберите строку для удаления");
    }
}

void MainWindow::onApplyZonesClicked()
{
    QVector<DetectionZone> zones;
    int rows = m_zoneTable->rowCount();
    bool ok;
    for (int i = 0; i < rows; ++i) {
        double minAng = m_zoneTable->item(i, 0)->text().toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "Ошибка", QString("Неверное значение в строке %1, столбец 'Начальный угол'").arg(i+1));
            return;
        }
        double maxAng = m_zoneTable->item(i, 1)->text().toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "Ошибка", QString("Неверное значение в строке %1, столбец 'Конечный угол'").arg(i+1));
            return;
        }
        int minDist = m_zoneTable->item(i, 2)->text().toInt(&ok);
        if (!ok) {
            QMessageBox::warning(this, "Ошибка", QString("Неверное значение в строке %1, столбец 'Мин дальность'").arg(i+1));
            return;
        }
        int maxDist = m_zoneTable->item(i, 3)->text().toInt(&ok);
        if (!ok) {
            QMessageBox::warning(this, "Ошибка", QString("Неверное значение в строке %1, столбец 'Макс дальность'").arg(i+1));
            return;
        }
        // Проверка корректности диапазонов
        if (minAng > maxAng) {
            QMessageBox::warning(this, "Ошибка", QString("В строке %1 минимальный угол больше максимального").arg(i+1));
            return;
        }
        if (minDist > maxDist) {
            QMessageBox::warning(this, "Ошибка", QString("В строке %1 минимальная дальность больше максимальной").arg(i+1));
            return;
        }
        zones.append(DetectionZone(minAng, maxAng, minDist, maxDist));
    }

    m_device->setZones(zones);
    appendData(tr(">>> Установлено %1 целей\n").arg(zones.size()).toUtf8(), false);
}

void MainWindow::onLoadExampleZonesClicked()
{
    // Предопределённый массив примеров (три зоны)
    static const struct { double minAng; double maxAng; int minDist; int maxDist; } examples[] = {
                    {42.9, 47.5, 148, 152},
                    {119.6, 127.8, 274, 276},
                    {199.5, 200.5, 50, 50},
                    {295.5, 305.5, 378, 382},
                    {6.5, 13.9, 198, 202},
                    {84.7, 87.1, 99, 101},
                    {145.5, 155.1, 318, 322},
                    {267.5, 272.5, 50, 50},
                    {30.0, 36.6, 178, 182},
                    {67.0, 68.6, 249, 251},
                    {105.5, 115.5, 298, 302},
                    {185.9, 194.3, 148, 152},
                    {238.8, 242.6, 219, 221},
                    {347.0, 353.0, 388, 392},
                    {2.9, 8.3, 80, 80},
                    {75.7, 84.7, 128, 132},
                    {134.7, 136.9, 259, 261},
                    {205.3, 215.3, 178, 182},
                    {277.7, 282.3, 308, 312},
                    {11.6, 19.4, 89, 91},
                    {95.1, 95.5, 200, 200},
                    {166.2, 175.6, 348, 352},
                    {227.6, 233.2, 119, 121},
                    {316.6, 323.6, 278, 282},
                    {36.0, 44.0, 396, 400}
    };
    const int exampleCount = sizeof(examples) / sizeof(examples[0]);

    // Очищаем таблицу
    m_zoneTable->setRowCount(0);

    // Заполняем примерами
    for (int i = 0; i < exampleCount; ++i) {
        int row = m_zoneTable->rowCount();
        m_zoneTable->insertRow(row);
        m_zoneTable->setItem(row, 0, new QTableWidgetItem(QString::number(examples[i].minAng, 'f', 1)));
        m_zoneTable->setItem(row, 1, new QTableWidgetItem(QString::number(examples[i].maxAng, 'f', 1)));
        m_zoneTable->setItem(row, 2, new QTableWidgetItem(QString::number(examples[i].minDist)));
        m_zoneTable->setItem(row, 3, new QTableWidgetItem(QString::number(examples[i].maxDist)));
    }

    // Сообщаем пользователю, что нужно применить зоны
    appendData(tr(">>> Загружены примеры целей. Нажмите 'Применить' для активации.\n").toUtf8(), false);
}

void MainWindow::onClearAllZonesClicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Внимание!", "Удалить все цели из списка??",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_zoneTable->setRowCount(0);
        appendData(">>> Все цели удалены. Нажмите 'Применить' для активации.\n", false);
    }
}

