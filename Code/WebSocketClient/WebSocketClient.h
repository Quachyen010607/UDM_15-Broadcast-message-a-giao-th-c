#pragma once

#include <QAbstractSocket>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QWebSocket>
#include <QTimer>

class WebSocketClient final : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClient(QObject *parent = nullptr);

    void connectToServer(const QUrl &url);
    void disconnectFromServer();
    void setAutoReconnectEnabled(bool enabled);
    [[nodiscard]] bool autoReconnectEnabled() const;
    [[nodiscard]] bool isConnected() const;
    bool sendBroadcastText(const QString &text);

signals:
    void stateChanged(QAbstractSocket::SocketState state);
    void connected();
    void disconnected();
    void rawTextMessageReceived(const QString &message);
    void broadcastReceived(const QString &senderId, const QString &messageId,
                           const QString &text, const QString &sentAt);
    void broadcastSent(const QString &text);
    void errorOccurred(const QString &message);
    void reconnectScheduled(int delayMs, int attempt);

private:
    void handleTextMessage(const QString &message);
    void scheduleReconnect();
    void openLastUrl();

    QWebSocket m_socket;
    QTimer m_reconnectTimer;
    QUrl m_lastUrl;
    bool m_shouldStayConnected = false;
    bool m_autoReconnectEnabled = true;
    int m_reconnectAttempts = 0;
};
