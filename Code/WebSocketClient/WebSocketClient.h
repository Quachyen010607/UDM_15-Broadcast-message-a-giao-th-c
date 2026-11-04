#pragma once

#include <QAbstractSocket>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QWebSocket>

class WebSocketClient final : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClient(QObject *parent = nullptr);

    void connectToServer(const QUrl &url);
    void disconnectFromServer();
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

private:
    void handleTextMessage(const QString &message);

    QWebSocket m_socket;
};
