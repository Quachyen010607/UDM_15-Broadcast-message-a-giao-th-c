#include "WebSocketClient.h"
#include "MessageProtocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QtGlobal>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, &WebSocketClient::openLastUrl);

    connect(&m_socket, &QWebSocket::stateChanged,
            this, &WebSocketClient::stateChanged);
    connect(&m_socket, &QWebSocket::connected, this, [this]() {
        m_reconnectTimer.stop();
        m_reconnectAttempts = 0;
        emit connected();
    });
    connect(&m_socket, &QWebSocket::disconnected, this, [this]() {
        emit disconnected();
        scheduleReconnect();
    });
    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::handleTextMessage);
    connect(&m_socket, &QWebSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket.errorString());
        QTimer::singleShot(0, this, [this]() { scheduleReconnect(); });
    });
}

void WebSocketClient::connectToServer(const QUrl &url)
{
    if (!url.isValid() || url.host().isEmpty() ||
        (url.scheme() != QStringLiteral("ws") && url.scheme() != QStringLiteral("wss"))) {
        emit errorOccurred(QStringLiteral("URL must use ws:// or wss:// and include a host"));
        return;
    }

    m_lastUrl = url;
    m_shouldStayConnected = true;
    m_reconnectTimer.stop();
    m_reconnectAttempts = 0;

    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();
    m_socket.open(m_lastUrl);
}

void WebSocketClient::disconnectFromServer()
{
    m_shouldStayConnected = false;
    m_reconnectTimer.stop();
    m_reconnectAttempts = 0;

    if (m_socket.state() == QAbstractSocket::ConnectedState ||
        m_socket.state() == QAbstractSocket::ConnectingState) {
        m_socket.close(QWebSocketProtocol::CloseCodeNormal, QStringLiteral("Client disconnect"));
    }
}

void WebSocketClient::setAutoReconnectEnabled(bool enabled)
{
    if (m_autoReconnectEnabled == enabled)
        return;

    m_autoReconnectEnabled = enabled;
    if (!enabled) {
        m_reconnectTimer.stop();
        return;
    }

    scheduleReconnect();
}

bool WebSocketClient::autoReconnectEnabled() const
{
    return m_autoReconnectEnabled;
}

bool WebSocketClient::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

bool WebSocketClient::sendBroadcastText(const QString &text)
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("Cannot send: WebSocket is not connected"));
        return false;
    }

    const QString request = QString::fromUtf8(
        MessageProtocol::encode(MessageProtocol::makeBroadcastRequest(text)));
    if (m_socket.sendTextMessage(request) < 0) {
        emit errorOccurred(m_socket.errorString());
        return false;
    }

    emit broadcastSent(text);
    return true;
}

void WebSocketClient::handleTextMessage(const QString &message)
{
    emit rawTextMessageReceived(message);

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit errorOccurred(QStringLiteral("Server sent an invalid JSON message"));
        return;
    }

    const QJsonObject event = document.object();
    const QJsonValue payloadValue = event.value(QStringLiteral("payload"));
    if (event.value(QStringLiteral("version")).toInt(-1) != MessageProtocol::Version ||
        event.value(QStringLiteral("type")).toString() != QStringLiteral("broadcast") ||
        !payloadValue.isObject() ||
        !payloadValue.toObject().value(QStringLiteral("text")).isString()) {
        emit errorOccurred(QStringLiteral("Server message does not match broadcast protocol v1"));
        return;
    }

    emit broadcastReceived(event.value(QStringLiteral("senderId")).toString(),
                           event.value(QStringLiteral("messageId")).toString(),
                           payloadValue.toObject().value(QStringLiteral("text")).toString(),
                           event.value(QStringLiteral("sentAt")).toString());
}

void WebSocketClient::scheduleReconnect()
{
    if (!m_shouldStayConnected || !m_autoReconnectEnabled || m_reconnectTimer.isActive() ||
        !m_lastUrl.isValid() || m_socket.state() != QAbstractSocket::UnconnectedState) {
        return;
    }

    int delayMs = 1000;
    for (int i = 0; i < m_reconnectAttempts && delayMs < 30000; ++i)
        delayMs = qMin(delayMs * 2, 30000);

    ++m_reconnectAttempts;
    emit reconnectScheduled(delayMs, m_reconnectAttempts);
    m_reconnectTimer.start(delayMs);
}

void WebSocketClient::openLastUrl()
{
    if (!m_shouldStayConnected || !m_autoReconnectEnabled ||
        m_socket.state() != QAbstractSocket::UnconnectedState) {
        return;
    }

    m_socket.open(m_lastUrl);
}
