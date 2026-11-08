#include "MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("UDM_15 - TCP Client (QTcpSocket)");
    resize(700, 500);

    // --- Hang 1: dia chi server ---
    m_hostEdit = new QLineEdit("127.0.0.1");
    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(5000);
    m_connectButton = new QPushButton("Connect");
    m_reconnectCheck = new QCheckBox("Tu dong ket noi lai");
    m_statusLabel = new QLabel;

    QHBoxLayout* serverRow = new QHBoxLayout;
    serverRow->addWidget(new QLabel("Server:"));
    serverRow->addWidget(m_hostEdit, 1);
    serverRow->addWidget(new QLabel("Port:"));
    serverRow->addWidget(m_portSpin);
    serverRow->addWidget(m_connectButton);
    serverRow->addWidget(m_reconnectCheck);

    // --- Hang 2: gui message ---
    m_messageEdit = new QLineEdit;
    m_messageEdit->setPlaceholderText("Nhap noi dung gui len server...");
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({ "Text", "Alert", "Info" });
    m_sendButton = new QPushButton("Gui");

    QHBoxLayout* sendRow = new QHBoxLayout;
    sendRow->addWidget(new QLabel("Noi dung:"));
    sendRow->addWidget(m_messageEdit, 1);
    sendRow->addWidget(m_typeCombo);
    sendRow->addWidget(m_sendButton);

    // --- Khung hien thi message ---
    m_logEdit = new QPlainTextEdit;
    m_logEdit->setReadOnly(true);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->addLayout(serverRow);
    root->addWidget(m_statusLabel);
    root->addLayout(sendRow);
    root->addWidget(m_logEdit, 1);

    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSend);
    connect(m_messageEdit, &QLineEdit::returnPressed, this, &MainWindow::onSend);
    connect(m_reconnectCheck, &QCheckBox::toggled, &m_client, &TcpClient::setAutoReconnect);
    connect(&m_client, &TcpClient::connected, this, &MainWindow::onConnected);
    connect(&m_client, &TcpClient::disconnected, this, &MainWindow::onDisconnected);
    connect(&m_client, &TcpClient::stateChanged, this, &MainWindow::onStateChanged);
    connect(&m_client, &TcpClient::messageReceived, this, &MainWindow::onMessageReceived);
    connect(&m_client, &TcpClient::errorOccurred, this, &MainWindow::onError);
    connect(&m_client, &TcpClient::reconnectScheduled, this, &MainWindow::onReconnectScheduled);

    onStateChanged(QAbstractSocket::UnconnectedState);
}

void MainWindow::onConnectClicked()
{
    if (m_client.state() == QAbstractSocket::UnconnectedState)
    {
        m_client.connectToServer(m_hostEdit->text().trimmed(), static_cast<quint16>(m_portSpin->value()));
    }
    else
    {
        m_client.disconnectFromServer();
    }
}

void MainWindow::onSend()
{
    QString text = m_messageEdit->text().trimmed();
    if (text.isEmpty())
    {
        return;
    }

    Message msg = Message::create(text, m_typeCombo->currentText());
    if (m_client.sendMessage(msg))
    {
        appendLog("Da gui: (" + msg.type + ") #" + msg.id + ": " + msg.content);
        m_messageEdit->clear();
    }
    else
    {
        appendLog("Gui that bai: chua ket noi toi server.");
    }
    m_messageEdit->setFocus();
}

// ---- Su kien ket noi / ngat ket noi ----
void MainWindow::onConnected()
{
    appendLog("Da ket noi toi server " + m_hostEdit->text() + ":" + QString::number(m_portSpin->value()));
}

void MainWindow::onDisconnected()
{
    appendLog("Da ngat ket noi.");
}

void MainWindow::onReconnectScheduled(int seconds)
{
    appendLog("Se thu ket noi lai sau " + QString::number(seconds) + " giay...");
}

void MainWindow::onStateChanged(QAbstractSocket::SocketState state)
{
    bool connected = (state == QAbstractSocket::ConnectedState);
    bool unconnected = (state == QAbstractSocket::UnconnectedState);

    m_hostEdit->setEnabled(unconnected);
    m_portSpin->setEnabled(unconnected);
    m_sendButton->setEnabled(connected);
    m_messageEdit->setEnabled(connected);
    m_typeCombo->setEnabled(connected);
    m_connectButton->setText(unconnected ? "Connect" : "Disconnect");

    if (connected)
    {
        m_statusLabel->setText("Trang thai: DA KET NOI");
    }
    else if (unconnected)
    {
        m_statusLabel->setText("Trang thai: CHUA KET NOI");
    }
    else
    {
        m_statusLabel->setText("Trang thai: DANG KET NOI...");
    }
}

void MainWindow::onMessageReceived(const Message& msg, const QString& rawLine)
{
    if (msg.id.isEmpty())
    {
        // Khong phai JSON hop le -> hien nguyen van
        appendLog("Nhan: " + rawLine);
        return;
    }
    appendLog("Nhan: [" + msg.sentTime.toString("HH:mm:ss") + "] (" + msg.type + ") #" + msg.id + ": " + msg.content);
}

void MainWindow::onError(const QString& text)
{
    appendLog("Loi: " + text);
}

void MainWindow::appendLog(const QString& text)
{
    m_logEdit->appendPlainText("[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] " + text);
}
