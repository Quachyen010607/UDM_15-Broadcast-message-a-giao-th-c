#pragma once

#include <QHostAddress>
#include <QObject>
#include <QSet>
#include <QString>
#include <QWebSocket>
#include <QWebSocketServer>

class WebSocketServer final : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketServer(QObject *parent = nullptr);
    ~WebSocketServer() override;

    bool listen(const QHostAddress &address, quint16 port);
    void close();

    [[nodiscard]] bool isListening() const;
    [[nodiscard]] qsizetype clientCount() const;
    [[nodiscard]] quint16 serverPort() const;

    void broadcastTextMessage(const QString &message);

signals:
    void clientConnected(const QString &peer);
    void clientDisconnected(const QString &peer);
    void textMessageReceived(const QString &peer, const QString &message);
    void serverError(const QString &message);

private slots:
    void acceptPendingConnections();

private:
    void attachClient(QWebSocket *client);
    [[nodiscard]] static QString peerLabel(const QWebSocket *client);

    QWebSocketServer m_server;
    QSet<QWebSocket *> m_clients;
};
