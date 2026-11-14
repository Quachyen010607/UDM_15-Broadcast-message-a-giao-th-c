#pragma once

#include "../common/ClientInfo.h"
#include "Message.h"

#include <QAbstractSocket>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

// Client TCP dua tren QTcpSocket: ket noi, gui va nhan message tu Server
class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();

    bool isConnected() const;
    QAbstractSocket::SocketState state() const;
    const ClientInfo &clientInfo() const;

    // Gui 1 message (JSON + '\n') len Server
    bool sendMessage(const Message& msg);

    // Tu dong ket noi lai sau 3 giay neu mat ket noi (khong ap dung khi nguoi dung chu dong ngat)
    void setAutoReconnect(bool enabled);

signals:
    void connected();
    void disconnected();
    void stateChanged(QAbstractSocket::SocketState state);
    void messageReceived(const Message& msg, const QString& rawLine);
    void errorOccurred(const QString& text);
    void reconnectScheduled(int seconds);

private slots:
    void onReadyRead();
    void onSocketError();
    void onSocketStateChanged(QAbstractSocket::SocketState state);
    void onRetryTimeout();

private:
    QTcpSocket m_socket;
    QTimer m_retryTimer;
    QString m_host;
    quint16 m_port = 0;
    QString m_clientId;
    ClientInfo m_clientInfo;
    bool m_autoReconnect = false;
    bool m_userDisconnect = false;
    bool m_wasConnected = false;
};
