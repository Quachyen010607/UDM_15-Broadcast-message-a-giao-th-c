#include "WebSocketServer.h"

#include <QAbstractSocket>
#include <utility>

WebSocketServer::WebSocketServer(QObject *parent)
    : QObject(parent)
    , m_server(QStringLiteral("UDM_15 Broadcast Server"),
               QWebSocketServer::NonSecureMode)
{
    connect(&m_server, &QWebSocketServer::newConnection,
            this, &WebSocketServer::acceptPendingConnections);

    connect(&m_server, &QWebSocketServer::acceptError,
            this, [this](QAbstractSocket::SocketError) {
                emit serverError(m_server.errorString());
            });

    connect(&m_server, &QWebSocketServer::serverError,
            this, [this](QWebSocketProtocol::CloseCode) {
                emit serverError(m_server.errorString());
            });
}

WebSocketServer::~WebSocketServer()
{
    close();
}

bool WebSocketServer::listen(const QHostAddress &address, quint16 port)
{
    if (m_server.isListening())
        close();

    if (m_server.listen(address, port))
        return true;

    emit serverError(m_server.errorString());
    return false;
}

void WebSocketServer::close()
{
    m_server.close();

    const auto clients = m_clients;
    for (QWebSocket *client : clients) {
        if (client)
            client->close(QWebSocketProtocol::CloseCodeGoingAway,
                          QStringLiteral("Server is shutting down"));
    }
}

bool WebSocketServer::isListening() const
{
    return m_server.isListening();
}

qsizetype WebSocketServer::clientCount() const
{
    return m_clients.size();
}

quint16 WebSocketServer::serverPort() const
{
    return m_server.serverPort();
}

void WebSocketServer::broadcastTextMessage(const QString &message)
{
    for (QWebSocket *client : std::as_const(m_clients)) {
        if (client && client->state() == QAbstractSocket::ConnectedState)
            client->sendTextMessage(message);
    }
}

void WebSocketServer::acceptPendingConnections()
{
    while (m_server.hasPendingConnections()) {
        QWebSocket *client = m_server.nextPendingConnection();
        if (!client)
            continue;

        attachClient(client);
    }
}

void WebSocketServer::attachClient(QWebSocket *client)
{
    client->setParent(this);
    m_clients.insert(client);

    const QString peer = peerLabel(client);
    emit clientConnected(peer);

    connect(client, &QWebSocket::textMessageReceived,
            this, [this, client](const QString &message) {
                emit textMessageReceived(peerLabel(client), message);
                broadcastTextMessage(message);
            });

    connect(client, &QWebSocket::disconnected,
            this, [this, client, peer] {
                m_clients.remove(client);
                emit clientDisconnected(peer);
                client->deleteLater();
            });

    connect(client, &QWebSocket::errorOccurred,
            this, [this, client](QAbstractSocket::SocketError) {
                emit serverError(client->errorString());
            });
}

QString WebSocketServer::peerLabel(const QWebSocket *client)
{
    if (!client)
        return {};

    return QStringLiteral("%1:%2")
        .arg(client->peerAddress().toString())
        .arg(client->peerPort());
}
