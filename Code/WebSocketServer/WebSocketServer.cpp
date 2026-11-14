#include "WebSocketServer.h"
#include "MessageProtocol.h"

#include <QAbstractSocket>

WebSocketServer::WebSocketServer(qsizetype maxConnections, QObject *parent)
    : QObject(parent),
      m_server(QStringLiteral("UDM_15 Broadcast Server"), QWebSocketServer::NonSecureMode),
      m_maxConnections(maxConnections > 0 ? maxConnections : 1)
{
    connect(&m_server, &QWebSocketServer::newConnection,
            this, &WebSocketServer::acceptPendingConnections);
    connect(&m_server, &QWebSocketServer::acceptError, this,
            [this](QAbstractSocket::SocketError) { emit serverError(m_server.errorString()); });
    connect(&m_server, &QWebSocketServer::serverError, this,
            [this](QWebSocketProtocol::CloseCode) { emit serverError(m_server.errorString()); });
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

    const auto sockets = m_clients.keys();
    for (QWebSocket *socket : sockets) {
        if (!socket)
            continue;

        // Remove bookkeeping synchronously so observers see an empty server on return.
        removeClient(socket);
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->close(QWebSocketProtocol::CloseCodeGoingAway,
                          QStringLiteral("Server is shutting down"));
        }
        socket->deleteLater();
    }
}

bool WebSocketServer::isListening() const { return m_server.isListening(); }
qsizetype WebSocketServer::clientCount() const { return m_clients.size(); }
quint16 WebSocketServer::serverPort() const { return m_server.serverPort(); }

QStringList WebSocketServer::clientIds() const
{
    QStringList ids;
    ids.reserve(m_clients.size());
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it)
        ids.append(it->clientId());
    return ids;
}

QList<ClientInfo> WebSocketServer::clientsInfo() const
{
    QList<ClientInfo> clients;
    clients.reserve(m_clients.size());
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it)
        clients.append(it.value());
    return clients;
}

qsizetype WebSocketServer::broadcastText(const QString &senderId, const QString &text)
{
    const QString wireText = QString::fromUtf8(
        MessageProtocol::encode(MessageProtocol::makeBroadcastEvent(senderId, text)));
    qsizetype delivered = 0;
    const auto sockets = m_clients.keys();
    for (QWebSocket *socket : sockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState &&
            socket->sendTextMessage(wireText) >= 0) {
            ++delivered;
        }
    }
    return delivered;
}

void WebSocketServer::acceptPendingConnections()
{
    while (m_server.hasPendingConnections()) {
        QWebSocket *socket = m_server.nextPendingConnection();
        if (!socket)
            continue;

        if (m_clients.size() >= m_maxConnections) {
            socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                          QStringLiteral("Maximum client count reached"));
            socket->deleteLater();
            continue;
        }
        attachClient(socket);
    }
}

void WebSocketServer::attachClient(QWebSocket *socket)
{
    if (!socket || m_clients.contains(socket))
        return;

    const ClientInfo info(QStringLiteral("client-%1").arg(++m_nextClientId),
                          peerLabel(socket));
    m_clients.insert(socket, info);

    connect(socket, &QWebSocket::stateChanged, this,
            [this, socket](QAbstractSocket::SocketState state) {
        const auto it = m_clients.constFind(socket);
        if (it != m_clients.cend())
            emit clientStateChanged(it->clientId(), state);
    });
    connect(socket, &QWebSocket::errorOccurred, this,
            [this, socket](QAbstractSocket::SocketError) {
        const auto it = m_clients.constFind(socket);
        if (it != m_clients.cend())
            emit clientError(it->clientId(), socket->errorString());
    });
    connect(socket, &QWebSocket::textMessageReceived, this,
            [this, socket](const QString &wireText) {
        const auto it = m_clients.constFind(socket);
        if (it == m_clients.cend())
            return;

        QString text;
        QString error;
        if (!MessageProtocol::parseBroadcastRequest(wireText, &text, &error)) {
            emit messageRejected(it->clientId(), error);
            return;
        }
        emit textMessageReceived(it->clientId(), text);
        broadcastText(it->clientId(), text);
    });
    connect(socket, &QWebSocket::disconnected, this, [this, socket]() {
        if (removeClient(socket))
            socket->deleteLater();
    });
    connect(socket, &QObject::destroyed, this, [this, socket]() {
        // Covers unexpected destruction without dereferencing the dying socket.
        removeClient(socket);
    });

    emit clientConnected(info.clientId(), info.peerAddress());
    emit clientStateChanged(info.clientId(), socket->state());
    emit clientCountChanged(m_clients.size());
}

bool WebSocketServer::removeClient(QWebSocket *socket)
{
    const auto it = m_clients.find(socket);
    if (it == m_clients.end())
        return false;

    const QString id = it->clientId();
    const QString peer = it->peerAddress();
    m_clients.erase(it);
    emit clientDisconnected(id, peer);
    emit clientCountChanged(m_clients.size());
    return true;
}

QString WebSocketServer::peerLabel(const QWebSocket *socket)
{
    return QStringLiteral("%1:%2")
        .arg(socket->peerAddress().toString())
        .arg(socket->peerPort());
}
