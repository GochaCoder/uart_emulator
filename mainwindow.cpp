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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_device(new EmulatedDevice(this))
    , m_portOpen(false)
{
    setWindowTitle("UART Emulator / Terminal");

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Группа настроек порта
    QGroupBox *portGroup = new QGroupBox("Настройки порта");
    QHBoxLayout *portLayout = new QHBoxLayout(portGroup);

    portLayout->addWidget(new QLabel("Порт:"));
    m_portCombo = new QComboBox;
    fillPortsInfo();
    portLayout->addWidget(m_portCombo);

    // Кнопка обновления списка портов
    m_refreshButton = new QPushButton("Обновить");
    portLayout->addWidget(m_refreshButton);

    portLayout->addWidget(new QLabel("Скорость:"));
    m_baudCombo = new QComboBox;
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "230400"});
    m_baudCombo->setCurrentText("115200");
    portLayout->addWidget(m_baudCombo);

    m_openCloseButton = new QPushButton("Открыть порт");
    portLayout->addWidget(m_openCloseButton);

    m_emulationCheckBox = new QCheckBox("Режим эмуляции (обработка команд)");
    m_emulationCheckBox->setChecked(true);
    portLayout->addWidget(m_emulationCheckBox);

    mainLayout->addWidget(portGroup);

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
    // Сохраняем текущий выбранный порт
    QString currentPort = m_portCombo->currentText();

    fillPortsInfo();

    // Восстанавливаем выбор, если порт всё ещё существует
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
            m_refreshButton->setEnabled(false);  // блокируем кнопку обновления
            appendData(tr(">>> Порт %1 открыт, скорость %2\n").arg(portName).arg(baud).toUtf8(), false);
        }
    } else {
        m_device->closePort();
        m_portOpen = false;
        m_openCloseButton->setText("Открыть порт");
        m_sendButton->setEnabled(false);
        m_portCombo->setEnabled(true);
        m_baudCombo->setEnabled(true);
        m_refreshButton->setEnabled(true);       // разблокируем
        appendData(">>> Порт закрыт\n", false);
    }
}

void MainWindow::onSendButtonClicked()
{
    QString text = m_commandEdit->text();
    if (text.isEmpty()) return;
    m_commandEdit->clear();

    // Добавляем символ новой строки, если его нет
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
    // Автопрокрутка
    QScrollBar *vbar = m_logEdit->verticalScrollBar();
    vbar->setValue(vbar->maximum());
}

void MainWindow::showError(const QString &error)
{
    QMessageBox::critical(this, "Ошибка последовательного порта", error);
    // Если порт был открыт, обновим состояние кнопки
    if (m_portOpen) {
        m_portOpen = false;
        m_openCloseButton->setText("Открыть порт");
        m_sendButton->setEnabled(false);
        m_portCombo->setEnabled(true);
        m_baudCombo->setEnabled(true);
        m_refreshButton->setEnabled(true);
    }
}
