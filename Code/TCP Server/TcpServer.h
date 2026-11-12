#pragma once

#include "../common/ClientInfo.h"
#include "Message.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>

// Ket qua gui cho 1 client
struct SendResult
{
    QString client;
    bool success = false;
    QString error;
};

// Quan ly cac ket noi TCP den tu Client, dua tren QTcpServer
class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(QObject* parent = nullptr);

    bool start(quint16 port);  // port = 0: he dieu hanh tu chon cong trong (dung cho test)
    void stop();
    bool isListening() const;
    quint16 port() const;
    QString errorString() const;

    QList<ClientInfo> clients() const;
    int clientCount() const;

    // Gui message den tat ca client dang online, tra ve ket qua tung client
    QList<SendResult> broadcast(const Message& msg);

    // Bat/tat che do chuyen tiep: message cua 1 client duoc gui cho cac client con lai,
    // dong thoi thong bao khi co client vao/ra
    void setRelayEnabled(bool enabled);
    bool relayEnabled() const;

signals:
    void clientConnected(const QString& address);
    void clientDisconnected(const QString& address);
    void clientCountChanged(int count);
    void logMessage(const QString& text);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QList<SendResult> sendTo(const QList<QTcpSocket*>& sockets, const QByteArray& data);
    void notifyOthers(QTcpSocket* except, const QString& text);

    QTcpServer m_server;
    QHash<QTcpSocket*, ClientInfo> m_clients;
    quint64 m_nextClientId = 0;
    QString m_error;
    bool m_relayEnabled = false;
    bool m_stopping = false;
};
