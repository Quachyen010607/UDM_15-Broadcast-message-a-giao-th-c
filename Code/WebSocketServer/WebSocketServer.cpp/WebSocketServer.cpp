#include "WebSocketServer.h"

#include <QAbstractSocket>
#include <QDebug>
#include <utility>

WebSocketServer::WebSocketServer(qsizetype maxConnections, QObject *parent)
    : QObject(parent)
    , m_server(QStringLiteral("UDM_15 Connection Manager"),
               QWebSocketServer::NonSecureMode)
    , m_maxConnections(maxConnections > 0 ? maxConnections : 1)
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
    const auto sockets = m_clients.keys();
    for (QWebSocket *socket : sockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->close(QWebSocketProtocol::CloseCodeGoingAway,
                          QStringLiteral("Server is shutting down"));
        }
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

QStringList WebSocketServer::clientIds() const
{
    QStringList ids;
    ids.reserve(m_clients.size());
    for (const ClientInfo &info : m_clients)
        ids.append(info.id);
    ids.sort();
    return ids;
}

quint16 WebSocketServer::serverPort() const
{
    return m_server.serverPort();
}

void WebSocketServer::acceptPendingConnections()
{
    while (m_server.hasPendingConnections()) {
        QWebSocket *socket = m_server.nextPendingConnection();
        if (!socket)
            continue;

        if (m_clients.size() >= m_maxConnections) {
            socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                          QStringLiteral("Server connection limit reached"));
            socket->deleteLater();
            continue;
        }

        attachClient(socket);
    }
}

void WebSocketServer::attachClient(QWebSocket *socket)
{
    const QString clientId = QStringLiteral("client-%1").arg(++m_nextClientId);
    const QString peer = peerLabel(socket);
    socket->setParent(this);
    m_clients.insert(socket, ClientInfo{clientId, peer, QDateTime::currentDateTimeUtc()});

    emit clientConnected(clientId, peer);
    emit clientCountChanged(m_clients.size());

    connect(socket, &QWebSocket::textMessageReceived,
            this, [this, clientId](const QString &message) {
                emit textMessageReceived(clientId, message);
            });
    connect(socket, &QWebSocket::disconnected,
            this, [this, socket] {
                const auto it = m_clients.find(socket);
                if (it == m_clients.end())
                    return;

                const ClientInfo info = it.value();
                m_clients.erase(it);
                emit clientDisconnected(info.id, info.peer);
                emit clientCountChanged(m_clients.size());
                socket->deleteLater();
            });
    connect(socket, &QWebSocket::errorOccurred,
            this, [this, socket, clientId](QAbstractSocket::SocketError) {
                emit serverError(QStringLiteral("%1: %2")
                                     .arg(clientId, socket->errorString()));
            });
}

QString WebSocketServer::peerLabel(const QWebSocket *socket)
{
    if (!socket)
        return {};
    return QStringLiteral("%1:%2")
        .arg(socket->peerAddress().toString())
        .arg(socket->peerPort());
}
