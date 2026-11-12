#include "MessageProtocol.h"
#include "../common/MessageParser.h"
#include "../common/MessageSerializer.h"
#include "../../UDP/src/UdpService.h"
#include "WebSocketServer.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUdpSocket>
#include <QWebSocket>

class MessageContractTests final : public QObject
{
    Q_OBJECT

private:
    static QByteArray validRequest()
    {
        const auto bytes = udm15::MessageSerializer::serialize(
            udm15::Message::broadcastRequest(QStringLiteral("contract-check")));
        Q_ASSERT(bytes.has_value());
        return *bytes;
    }

    static quint16 availableUdpPort()
    {
        QUdpSocket probe;
        if (!probe.bind(QHostAddress::LocalHost, 0))
            return 0;
        const quint16 port = probe.localPort();
        probe.close();
        return port;
    }

private slots:
    void sharedJsonHasStableRequestShape()
    {
        const QByteArray wire = validRequest();
        const QJsonDocument document = QJsonDocument::fromJson(wire);
        QVERIFY(document.isObject());
        const QJsonObject object = document.object();
        QCOMPARE(object.size(), 3);
        QCOMPARE(object.value(QStringLiteral("version")).toInt(),
                 udm15::MessageProtocolVersion);
        QCOMPARE(object.value(QStringLiteral("type")).toString(),
                 udm15::BroadcastMessageType);
        QVERIFY(object.value(QStringLiteral("payload")).isObject());
        QCOMPARE(object.value(QStringLiteral("payload")).toObject()
                     .value(QStringLiteral("text")).toString(),
                 QStringLiteral("contract-check"));

        QString error;
        const auto parsed = udm15::MessageParser::parse(wire, &error);
        QVERIFY2(parsed.has_value(), qPrintable(error));
        QCOMPARE(parsed->text, QStringLiteral("contract-check"));
        QVERIFY(error.isEmpty());
    }

    void eventMetadataRoundTripsThroughCommonJson()
    {
        const udm15::Message source = udm15::Message::broadcastEvent(
            QStringLiteral("client-7"), QStringLiteral("event-check"));
        QString error;
        const auto wire = udm15::MessageSerializer::serialize(source, &error);
        QVERIFY2(wire.has_value(), qPrintable(error));

        const auto parsed = udm15::MessageParser::parse(*wire, &error);
        QVERIFY2(parsed.has_value(), qPrintable(error));
        QCOMPARE(parsed->version, source.version);
        QCOMPARE(parsed->type, source.type);
        QCOMPARE(parsed->text, source.text);
        QCOMPARE(parsed->messageId, source.messageId);
        QCOMPARE(parsed->senderId, source.senderId);
        QCOMPARE(parsed->sentAt.toUTC(), source.sentAt.toUTC());
        QCOMPARE(QJsonDocument::fromJson(*wire).object().size(), 6);
    }

    void tcpLineAndUdpDatagramPreserveSharedJson()
    {
        const QByteArray wire = validRequest();
        const QByteArray tcpLine = wire + '\n';
        QString error;
        const auto tcpMessage = udm15::MessageParser::parse(tcpLine, &error);
        QVERIFY2(tcpMessage.has_value(), qPrintable(error));
        QCOMPARE(tcpMessage->text, QStringLiteral("contract-check"));

        const quint16 port = availableUdpPort();
        QVERIFY(port != 0);
        UdpService udp;
        QSignalSpy receivedSpy(&udp, &UdpService::messageReceived);
        QVERIFY(receivedSpy.isValid());
        QVERIFY(udp.startListening(port));
        QUdpSocket sender;
        QCOMPARE(sender.writeDatagram(wire, QHostAddress::LocalHost, port),
                 qint64(wire.size()));
        QTRY_COMPARE_WITH_TIMEOUT(receivedSpy.count(), 1, 3000);

        const QJsonObject payload = receivedSpy.first().at(0).toJsonObject();
        const auto udpMessage = udm15::MessageParser::parse(
            QJsonDocument(payload).toJson(QJsonDocument::Compact), &error);
        QVERIFY2(udpMessage.has_value(), qPrintable(error));
        QCOMPARE(udpMessage->text, tcpMessage->text);
        QCOMPARE(udpMessage->version, tcpMessage->version);
        QCOMPARE(udpMessage->type, tcpMessage->type);
        udp.stopListening();
    }

    void webSocketAcceptsTheSameRequestEnvelope()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QSignalSpy receivedSpy(&server, &WebSocketServer::textMessageReceived);
        QSignalSpy rejectedSpy(&server, &WebSocketServer::messageRejected);
        QWebSocket client;
        client.open(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort())));
        QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::ConnectedState, 3000);
        QVERIFY(client.sendTextMessage(QString::fromUtf8(validRequest())) >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(receivedSpy.count(), 1, 3000);
        QCOMPARE(rejectedSpy.count(), 0);
        QCOMPARE(receivedSpy.first().at(1).toString(), QStringLiteral("contract-check"));
        client.close();
    }

    void parserRejectsMalformedAndInvalidSchema()
    {
        QString error;
        QVERIFY(!udm15::MessageParser::parse(QByteArrayLiteral("{bad"), &error));
        QVERIFY(error.contains(QStringLiteral("Invalid JSON")));
        QVERIFY(!udm15::MessageParser::parse(
            QByteArrayLiteral("{\"version\":2,\"type\":\"broadcast\",\"payload\":{\"text\":\"x\"}}"),
            &error));
        QVERIFY(error.contains(QStringLiteral("version")));
    }
};

QTEST_GUILESS_MAIN(MessageContractTests)
#include "MessageContractTests.moc"
