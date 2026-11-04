#include "WebSocketClientWindow.h"

#include "WebSocketClient.h"

#include <QCheckBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

WebSocketClientWindow::WebSocketClientWindow(QWidget *parent)
    : QMainWindow(parent),
      m_client(new WebSocketClient(this)),
      m_urlEdit(new QLineEdit(QStringLiteral("ws://127.0.0.1:5002"), this)),
      m_autoReconnectCheck(new QCheckBox(QStringLiteral("Tự kết nối lại"), this)),
      m_statusLabel(new QLabel(QStringLiteral("Chưa kết nối"), this)),
      m_logEdit(new QPlainTextEdit(this)),
      m_messageEdit(new QLineEdit(this)),
      m_connectButton(new QPushButton(QStringLiteral("Kết nối"), this)),
      m_disconnectButton(new QPushButton(QStringLiteral("Ngắt kết nối"), this)),
      m_sendButton(new QPushButton(QStringLiteral("Gửi broadcast"), this))
{
    setWindowTitle(QStringLiteral("UDM 15 — WebSocket Client"));
    resize(760, 520);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    auto *connectionLayout = new QHBoxLayout;
    connectionLayout->addWidget(new QLabel(QStringLiteral("Server URL:"), central));
    connectionLayout->addWidget(m_urlEdit, 1);
    connectionLayout->addWidget(m_connectButton);
    connectionLayout->addWidget(m_disconnectButton);
    rootLayout->addLayout(connectionLayout);

    auto *statusLayout = new QHBoxLayout;
    statusLayout->addWidget(m_statusLabel, 1);
    m_autoReconnectCheck->setChecked(true);
    statusLayout->addWidget(m_autoReconnectCheck);
    rootLayout->addLayout(statusLayout);

    m_logEdit->setReadOnly(true);
    rootLayout->addWidget(m_logEdit, 1);

    auto *messageLayout = new QHBoxLayout;
    m_messageEdit->setPlaceholderText(QStringLiteral("Nhập nội dung broadcast..."));
    messageLayout->addWidget(m_messageEdit, 1);
    messageLayout->addWidget(m_sendButton);
    rootLayout->addLayout(messageLayout);
    setCentralWidget(central);

    connect(m_connectButton, &QPushButton::clicked, this, [this]() {
        m_client->connectToServer(QUrl(m_urlEdit->text().trimmed()));
    });
    connect(m_disconnectButton, &QPushButton::clicked,
            m_client, &WebSocketClient::disconnectFromServer);
    connect(m_sendButton, &QPushButton::clicked, this, &WebSocketClientWindow::sendMessage);
    connect(m_messageEdit, &QLineEdit::returnPressed,
            this, &WebSocketClientWindow::sendMessage);
    connect(m_autoReconnectCheck, &QCheckBox::toggled,
            m_client, &WebSocketClient::setAutoReconnectEnabled);
    connect(m_client, &WebSocketClient::stateChanged,
            this, &WebSocketClientWindow::updateConnectionControls);
    connect(m_client, &WebSocketClient::connected, this, [this]() {
        appendLog(QStringLiteral("Đã kết nối tới %1").arg(m_urlEdit->text().trimmed()));
    });
    connect(m_client, &WebSocketClient::disconnected, this, [this]() {
        appendLog(QStringLiteral("Kết nối đã đóng."));
    });
    connect(m_client, &WebSocketClient::broadcastSent, this, [this](const QString &text) {
        appendLog(QStringLiteral("Đã gửi: %1").arg(text));
    });
    connect(m_client, &WebSocketClient::broadcastReceived, this,
            [this](const QString &senderId, const QString &messageId,
                   const QString &text, const QString &sentAt) {
        appendLog(QStringLiteral("Broadcast từ %1 [%2, %3]: %4")
                      .arg(senderId, messageId, sentAt, text));
    });
    connect(m_client, &WebSocketClient::errorOccurred, this, [this](const QString &message) {
        appendLog(QStringLiteral("Lỗi: %1").arg(message));
    });
    connect(m_client, &WebSocketClient::reconnectScheduled, this,
            [this](int delayMs, int attempt) {
        appendLog(QStringLiteral("Mất kết nối. Thử lại lần %1 sau %2 giây.")
                      .arg(attempt).arg(delayMs / 1000.0, 0, 'f', 1));
    });

    updateConnectionControls(QAbstractSocket::UnconnectedState);
}

void WebSocketClientWindow::updateConnectionControls(QAbstractSocket::SocketState state)
{
    const bool connected = state == QAbstractSocket::ConnectedState;
    const bool busy = state == QAbstractSocket::ConnectingState ||
                      state == QAbstractSocket::ClosingState;
    m_statusLabel->setText(connected ? QStringLiteral("Đã kết nối")
                                     : busy ? QStringLiteral("Đang kết nối/ngắt...")
                                            : QStringLiteral("Chưa kết nối"));
    m_connectButton->setEnabled(!busy && !connected);
    m_urlEdit->setEnabled(!busy && !connected);
    m_disconnectButton->setEnabled(connected || busy);
    m_sendButton->setEnabled(connected);
}

void WebSocketClientWindow::appendLog(const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_logEdit->appendPlainText(QStringLiteral("[%1] %2").arg(time, message));
}

void WebSocketClientWindow::sendMessage()
{
    const QString text = m_messageEdit->text().trimmed();
    if (text.isEmpty())
        return;
    if (m_client->sendBroadcastText(text))
        m_messageEdit->clear();
}
