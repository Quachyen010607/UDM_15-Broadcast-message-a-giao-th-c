#include "UdpService.h"

#include "../../Code/common/Message.h"
#include "../../Code/common/MessageSerializer.h"

#include <QJsonDocument>

UdpService::UdpService(QObject *parent) : QObject(parent) {
    connect(&m_receiver, &QUdpSocket::readyRead,
            this, &UdpService::readPendingDatagrams);
    connect(&m_receiver, &QUdpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) { emit errorOccurred(m_receiver.errorString()); });
    connect(&m_sender, &QUdpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) { emit errorOccurred(m_sender.errorString()); });
}

bool UdpService::startListening(quint16 port) {
    if (port == 0) {
        emit errorOccurred("Cổng nhận phải nằm trong khoảng 1..65535.");
        return false;
    }
    if (m_receiver.state() == QAbstractSocket::BoundState && m_receiver.localPort() == port)
        return true;
    m_receiver.close();
    // Listen on all IPv4 interfaces. ShareAddress allows multiple test apps to bind on some OSes;
    // whether each receives every broadcast depends on OS/socket behavior.
    if (!m_receiver.bind(QHostAddress::AnyIPv4, port,
                         QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit errorOccurred(QString("Không mở được cổng %1: %2").arg(port).arg(m_receiver.errorString()));
        return false;
    }
    emit statusChanged(QString("Đang nhận UDP tại cổng %1").arg(port));
    return true;
}

void UdpService::stopListening() {
    m_receiver.close();
    emit statusChanged("Đã dừng nhận UDP");
}

bool UdpService::isListening() const {
    return m_receiver.state() == QAbstractSocket::BoundState;
}

quint16 UdpService::listeningPort() const { return m_receiver.localPort(); }

bool UdpService::sendJson(const QJsonObject &payload, quint16 destinationPort,
                          const QHostAddress &destination) {
    return sendDatagram(QJsonDocument(payload).toJson(QJsonDocument::Compact),
                        destinationPort, destination);
}

bool UdpService::sendMessage(const QString &message, quint16 destinationPort,
                             const QHostAddress &destination) {
    QString error;
    const auto bytes = udm15::MessageSerializer::serialize(
        udm15::Message::broadcastRequest(message), &error);
    if (!bytes) {
        emit errorOccurred(error);
        return false;
    }
    return sendDatagram(*bytes, destinationPort, destination);
}

bool UdpService::sendDatagram(const QByteArray &bytes, quint16 destinationPort,
                              const QHostAddress &destination) {
    if (destinationPort == 0 || destination.isNull()) {
        emit errorOccurred("Địa chỉ đích hoặc cổng gửi không hợp lệ.");
        return false;
    }
    if (bytes.size() > 1200) {
        emit errorOccurred("Gói JSON lớn hơn 1200 byte; hãy chia nhỏ để tránh phân mảnh UDP.");
        return false;
    }
    const qint64 sent = m_sender.writeDatagram(bytes, destination, destinationPort);
    if (sent != bytes.size()) {
        emit errorOccurred(QString("Gửi UDP thất bại: %1").arg(m_sender.errorString()));
        return false;
    }
    emit statusChanged(QString("Đã gửi %1 byte đến %2:%3")
                       .arg(sent).arg(destination.toString()).arg(destinationPort));
    return true;
}

void UdpService::readPendingDatagrams() {
    // Drain the socket to avoid dropping packets under bursty traffic.
    while (m_receiver.hasPendingDatagrams()) {
        QByteArray data;
        data.resize(static_cast<int>(m_receiver.pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        const qint64 count = m_receiver.readDatagram(data.data(), data.size(), &sender, &senderPort);
        if (count < 0) {
            emit errorOccurred(QString("Lỗi nhận UDP: %1").arg(m_receiver.errorString()));
            continue;
        }
        data.resize(static_cast<int>(count));
        const QString ip = sender.toString();
        if (data.size() > 1200) {
            emit invalidDatagram("Gói quá 1200 byte", ip, senderPort);
            continue;
        }
        QString decodeError;
        const auto message = udm15::MessageSerializer::deserialize(data, &decodeError);
        if (!message) {
            emit invalidDatagram(decodeError, ip, senderPort);
            continue;
        }
        emit messageReceived(message->toJson(), ip, senderPort);
    }
}
