#pragma once

#include "../common/ClientInfo.h"

#include <QAbstractSocket>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QWebSocket>
#include <QWebSocketServer>

class WebSocketServer final : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketServer(qsizetype maxConnections = 64,
                             QObject *parent = nullptr);
    ~WebSocketServer() override;

    bool listen(const QHostAddress &address, quint16 port);
    void close();

    [[nodiscard]] bool isListening() const;
    [[nodiscard]] qsizetype clientCount() const;
    [[nodiscard]] QStringList clientIds() const;
    [[nodiscard]] QList<ClientInfo> clientsInfo() const;
    [[nodiscard]] quint16 serverPort() const;
    qsizetype broadcastText(const QString &senderId, const QString &text);

signals:
    void clientConnected(const QString &clientId, const QString &peer);
    void clientDisconnected(const QString &clientId, const QString &peer);
    void clientCountChanged(qsizetype count);
    void clientStateChanged(const QString &clientId, QAbstractSocket::SocketState state);
    void clientError(const QString &clientId, const QString &message);
    void textMessageReceived(const QString &clientId, const QString &message);
    void messageRejected(const QString &clientId, const QString &reason);
    void serverError(const QString &message);

private slots:
    void acceptPendingConnections();

private:
    void attachClient(QWebSocket *socket);
    bool removeClient(QWebSocket *socket);
    [[nodiscard]] static QString peerLabel(const QWebSocket *socket);

    QWebSocketServer m_server;
    QHash<QWebSocket *, ClientInfo> m_clients;
    quint64 m_nextClientId = 0;
    qsizetype m_maxConnections;
};
