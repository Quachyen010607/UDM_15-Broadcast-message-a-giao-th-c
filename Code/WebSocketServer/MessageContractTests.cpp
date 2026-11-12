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

    void webSocketBroadcastKeepsTheSharedMessageFields()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QSignalSpy receivedSpy(&server, &WebSocketServer::textMessageReceived);
        QSignalSpy rejectedSpy(&server, &WebSocketServer::messageRejected);
        QWebSocket client;
        QString broadcastWire;
        connect(&client, &QWebSocket::textMessageReceived, &client,
                [&broadcastWire](const QString &wire) { broadcastWire = wire; });
        client.open(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort())));
        QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::ConnectedState, 3000);
        QVERIFY(client.sendTextMessage(QString::fromUtf8(validRequest())) >= 0);
        QTRY_COMPARE_WITH_TIMEOUT(receivedSpy.count(), 1, 3000);
        QTRY_VERIFY_WITH_TIMEOUT(!broadcastWire.isEmpty(), 3000);
        QCOMPARE(rejectedSpy.count(), 0);
        QCOMPARE(receivedSpy.first().at(1).toString(), QStringLiteral("contract-check"));

        QString error;
        const auto request = udm15::MessageParser::parse(validRequest(), &error);
        QVERIFY2(request.has_value(), qPrintable(error));
        const auto event = udm15::MessageParser::parse(broadcastWire.toUtf8(), &error);
        QVERIFY2(event.has_value(), qPrintable(error));
        QCOMPARE(event->version, request->version);
        QCOMPARE(event->type, request->type);
        QCOMPARE(event->text, request->text);
        QVERIFY(!event->messageId.isEmpty());
        QCOMPARE(event->senderId, receivedSpy.first().at(0).toString());
        QVERIFY(event->sentAt.isValid());
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
