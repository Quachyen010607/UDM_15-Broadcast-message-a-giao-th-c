#include "TcpServer.h"

TcpServer::TcpServer(QObject* parent) : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

bool TcpServer::start(quint16 port)
{
    if (m_server.isListening())
        return true;

    if (!m_server.listen(QHostAddress::AnyIPv4, port)) {
        m_error = m_server.errorString();
        emit logMessage("Khong the mo cong " + QString::number(port) + ": " + m_error);
        return false;
    }

    emit logMessage("TCP Server dang lang nghe tai cong " + QString::number(m_server.serverPort()));
    return true;
}

void TcpServer::stop()
{
    // Khi dung server khong gui thong bao "client ngat" cho nhau
    m_stopping = true;
    const QList<QTcpSocket*> sockets = m_clients.keys();
    for (QTcpSocket* socket : sockets)
        socket->abort();
    m_server.close();
    m_stopping = false;

    emit logMessage("TCP Server da dung.");
}

bool TcpServer::isListening() const { return m_server.isListening(); }
quint16 TcpServer::port() const { return m_server.serverPort(); }
QString TcpServer::errorString() const { return m_error; }
QList<ClientInfo> TcpServer::clients() const { return m_clients.values(); }
int TcpServer::clientCount() const { return m_clients.size(); }

void TcpServer::setRelayEnabled(bool enabled) { m_relayEnabled = enabled; }
bool TcpServer::relayEnabled() const { return m_relayEnabled; }

void TcpServer::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket* socket = m_server.nextPendingConnection();
        if (!socket)
            continue;

        const QString address = socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());
        const ClientInfo info(QStringLiteral("tcp-%1").arg(++m_nextClientId), address);
        m_clients.insert(socket, info);

        connect(socket, &QTcpSocket::readyRead, this, &TcpServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &TcpServer::onDisconnected);
        connect(socket, &QAbstractSocket::errorOccurred, this,
                [this, info](QAbstractSocket::SocketError error) {
            if (error != QAbstractSocket::RemoteHostClosedError)
                emit logMessage("Loi socket " + info.peerAddress() + " (ma " + QString::number(error) + ")");
        });

        emit logMessage("Client ket noi: " + info.peerAddress() + " (tong: " + QString::number(m_clients.size()) + ")");
        emit clientConnected(info.peerAddress());
        emit clientCountChanged(m_clients.size());

        if (m_relayEnabled)
            notifyOthers(socket, "Client " + info.peerAddress() + " da ket noi");
    }
}

void TcpServer::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket == nullptr || !m_clients.contains(socket))
        return;

    const QString address = m_clients.take(socket).peerAddress();
    socket->deleteLater();

    emit logMessage("Client ngat ket noi: " + address + " (con lai: " + QString::number(m_clients.size()) + ")");
    emit clientDisconnected(address);
    emit clientCountChanged(m_clients.size());

    if (m_relayEnabled && !m_stopping)
        notifyOthers(nullptr, "Client " + address + " da ngat ket noi");
}

void TcpServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket == nullptr || !m_clients.contains(socket))
        return;

    const QString address = m_clients.value(socket).peerAddress();
    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        if (line.isEmpty())
            continue;

        Message msg = Message::fromJson(line);
        const QString text = msg.content.isEmpty() ? QString::fromUtf8(line) : msg.content;
        emit logMessage("Nhan tu " + address + ": " + text);

        if (m_relayEnabled) {
            Message out = Message::create("[" + address + "] " + text, msg.type.isEmpty() ? "Text" : msg.type);
            QList<QTcpSocket*> others = m_clients.keys();
            others.removeAll(socket);

            const QList<SendResult> results = sendTo(others, out.toJson() + QByteArray(1, char(10)));
            emit logMessage("Chuyen tiep tin cua " + address + " den " + QString::number(results.size()) + " client");
        }
    }
}

QList<SendResult> TcpServer::broadcast(const Message& msg)
{
    return sendTo(m_clients.keys(), msg.toJson() + QByteArray(1, char(10)));
}

QList<SendResult> TcpServer::sendTo(const QList<QTcpSocket*>& sockets, const QByteArray& data)
{
    QList<SendResult> results;
    for (QTcpSocket* socket : sockets) {
        if (!m_clients.contains(socket))
            continue;

        SendResult result;
        result.client = m_clients.value(socket).peerAddress();

        if (socket->state() != QAbstractSocket::ConnectedState) {
            result.success = false;
            result.error = "Client khong con ket noi";
        } else if (socket->write(data) == data.size()) {
            result.success = true;
        } else {
            result.success = false;
            result.error = socket->errorString();
            socket->abort();
        }
        results.append(result);
    }
    return results;
}

void TcpServer::notifyOthers(QTcpSocket* except, const QString& text)
{
    QList<QTcpSocket*> others = m_clients.keys();
    others.removeAll(except);
    sendTo(others, Message::create(text, "Info").toJson() + QByteArray(1, char(10)));
}
