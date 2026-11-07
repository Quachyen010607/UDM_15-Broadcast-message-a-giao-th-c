#pragma once

#include <QObject>
#include <QHostAddress>
#include <QJsonObject>
#include <QUdpSocket>

class UdpService : public QObject {
    Q_OBJECT
public:
    explicit UdpService(QObject *parent = nullptr);

    bool startListening(quint16 port);
    void stopListening();
    bool isListening() const;
    quint16 listeningPort() const;

    // Sends one UTF-8 JSON datagram to a destination (broadcast by default).
    bool sendJson(const QJsonObject &payload, quint16 destinationPort,
                  const QHostAddress &destination = QHostAddress::Broadcast);
    bool sendMessage(const QString &message, quint16 destinationPort,
                     const QHostAddress &destination = QHostAddress::Broadcast);

signals:
    void messageReceived(const QJsonObject &payload, const QString &senderIp, quint16 senderPort);
    void invalidDatagram(const QString &reason, const QString &senderIp, quint16 senderPort);
    void statusChanged(const QString &status);
    void errorOccurred(const QString &error);

private slots:
    void readPendingDatagrams();

private:
    QUdpSocket m_receiver;
    QUdpSocket m_sender;
};
