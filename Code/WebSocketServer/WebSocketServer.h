#pragma once

#include <QDateTime>
#include <QHash>
#include <QHostAddress>
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
    [[nodiscard]] quint16 serverPort() const;

signals:
    void clientConnected(const QString &clientId, const QString &peer);
    void clientDisconnected(const QString &clientId, const QString &peer);
    void clientCountChanged(qsizetype count);
    void textMessageReceived(const QString &clientId, const QString &message);
    void serverError(const QString &message);

private slots:
    void acceptPendingConnections();

private:
    struct ClientInfo {
        QString id;
        QString peer;
        QDateTime connectedAt;
    };

    void attachClient(QWebSocket *socket);
    [[nodiscard]] static QString peerLabel(const QWebSocket *socket);

    QWebSocketServer m_server;
    QHash<QWebSocket *, ClientInfo> m_clients;
    quint64 m_nextClientId = 0;
    qsizetype m_maxConnections;
};
