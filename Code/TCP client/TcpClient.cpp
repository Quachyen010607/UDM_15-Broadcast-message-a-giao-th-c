#include "TcpClient.h"

static const int RetrySeconds = 3;

TcpClient::TcpClient(QObject* parent) : QObject(parent)
{
    m_retryTimer.setSingleShot(true);
    m_retryTimer.setInterval(RetrySeconds * 1000);

    connect(&m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(&m_socket, &QAbstractSocket::stateChanged, this, &TcpClient::onSocketStateChanged);
    connect(&m_socket, &QAbstractSocket::errorOccurred, this, &TcpClient::onSocketError);
    connect(&m_retryTimer, &QTimer::timeout, this, &TcpClient::onRetryTimeout);
}

void TcpClient::connectToServer(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_userDisconnect = false;
    m_retryTimer.stop();

    if (m_socket.state() != QAbstractSocket::UnconnectedState)
    {
        m_socket.abort();
    }
    m_socket.connectToHost(m_host, m_port);
}

void TcpClient::disconnectFromServer()
{
    m_userDisconnect = true;
    m_retryTimer.stop();

    if (m_socket.state() == QAbstractSocket::ConnectedState)
    {
        m_socket.disconnectFromHost();
    }
    else if (m_socket.state() != QAbstractSocket::UnconnectedState)
    {
        m_socket.abort();  // dang ket noi: huy luon
    }
}

bool TcpClient::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

QAbstractSocket::SocketState TcpClient::state() const
{
    return m_socket.state();
}

void TcpClient::setAutoReconnect(bool enabled)
{
    m_autoReconnect = enabled;
    if (!enabled)
    {
        m_retryTimer.stop();
    }
}

bool TcpClient::sendMessage(const Message& msg)
{
    if (!isConnected())
    {
        return false;
    }

    QByteArray data = msg.toJson() + '\n';
    return m_socket.write(data) == data.size();
}

void TcpClient::onReadyRead()
{
    // Chi xu ly khi da du 1 dong (message co the den thanh nhieu manh TCP)
    while (m_socket.canReadLine())
    {
        QByteArray line = m_socket.readLine().trimmed();
        if (line.isEmpty())
        {
            continue;
        }
        emit messageReceived(Message::fromJson(line), QString::fromUtf8(line));
    }
}

void TcpClient::onSocketError()
{
    emit errorOccurred(m_socket.errorString());
}

// Su kien ket noi / ngat ket noi
void TcpClient::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    emit stateChanged(state);

    if (state == QAbstractSocket::ConnectedState)
    {
        m_wasConnected = true;
        emit connected();
    }
    else if (state == QAbstractSocket::UnconnectedState)
    {
        if (m_wasConnected)
        {
            m_wasConnected = false;
            emit disconnected();
        }

        // Mat ket noi ngoai y muon (server tat, mang loi...) hoac ket noi that bai -> thu lai
        if (m_autoReconnect && !m_userDisconnect)
        {
            m_retryTimer.start();
            emit reconnectScheduled(RetrySeconds);
        }
    }
}

void TcpClient::onRetryTimeout()
{
    if (m_autoReconnect && !m_userDisconnect && m_socket.state() == QAbstractSocket::UnconnectedState)
    {
        m_socket.connectToHost(m_host, m_port);
    }
}
