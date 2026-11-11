#include "WebSocketServer.h"
#include "MessageProtocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTest>
#include <QWebSocket>

#include <algorithm>

class WebSocketServerTests final : public QObject
{
    Q_OBJECT

private:
    static QUrl localUrl(const WebSocketServer &server)
    {
        return QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort()));
    }

private slots:
    void acceptsMultipleClientsAndRemovesEachOnce()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QSignalSpy connectedSpy(&server, &WebSocketServer::clientConnected);
        QSignalSpy disconnectedSpy(&server, &WebSocketServer::clientDisconnected);
        QSignalSpy stateSpy(&server, &WebSocketServer::clientStateChanged);

        QWebSocket first;
        QWebSocket second;
        first.open(localUrl(server));
        second.open(localUrl(server));

        QTRY_COMPARE_WITH_TIMEOUT(connectedSpy.count(), 2, 3000);
        QCOMPARE(server.clientCount(), 2);
        QString firstId;
        QString secondId;
        const QString firstPeerPort = QStringLiteral(":%1").arg(first.localPort());
        for (const auto &args : connectedSpy) {
            const QString clientId = args.at(0).toString();
            const QString peer = args.at(1).toString();
            if (peer.endsWith(firstPeerPort))
                firstId = clientId;
            else
                secondId = clientId;
        }
        QVERIFY(!firstId.isEmpty());
        QVERIFY(!secondId.isEmpty());
        QVERIFY(firstId != secondId);
        QTRY_VERIFY_WITH_TIMEOUT(
            std::any_of(stateSpy.cbegin(), stateSpy.cend(), [&firstId](const QList<QVariant> &args) {
                return args.at(0).toString() == firstId &&
                       args.at(1).value<QAbstractSocket::SocketState>() == QAbstractSocket::ConnectedState;
            }), 3000);

        first.close();
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(disconnectedSpy.count(), 1, 3000);
        first.abort(); // A second terminal event must not report the same client again.
        QTest::qWait(50);
        QCOMPARE(disconnectedSpy.count(), 1);
        QCOMPARE(disconnectedSpy.first().first().toString(), firstId);

        second.close();
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 0, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(disconnectedSpy.count(), 2, 3000);
    }

    void repeatedConnectAndDisconnectKeepsCountAccurate()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QSignalSpy disconnectedSpy(&server, &WebSocketServer::clientDisconnected);

        constexpr int cycles = 10;
        for (int i = 0; i < cycles; ++i) {
            QWebSocket client;
            client.open(localUrl(server));
            QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::ConnectedState, 3000);
            client.close();
            QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(disconnectedSpy.count(), i + 1, 3000);
        }

        QCOMPARE(disconnectedSpy.count(), cycles);
        QCOMPARE(server.clientIds().size(), 0);
    }

    void serverShutdownClearsAllTrackedClientsImmediately()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QSignalSpy disconnectedSpy(&server, &WebSocketServer::clientDisconnected);
        QWebSocket first;
        QWebSocket second;
        first.open(localUrl(server));
        second.open(localUrl(server));
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 2, 3000);

        server.close();

        QCOMPARE(server.clientCount(), 0);
        QVERIFY(server.clientIds().isEmpty());
        QVERIFY(!server.isListening());
        QCOMPARE(disconnectedSpy.count(), 2);
        first.abort();
        second.abort();
        QTest::qWait(50);
        QCOMPARE(disconnectedSpy.count(), 2);
    }

    void rejectsConnectionsOverConfiguredLimit()
    {
        WebSocketServer server(1);
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QWebSocket accepted;
        QWebSocket rejected;
        accepted.open(localUrl(server));
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(accepted.state(), QAbstractSocket::ConnectedState, 3000);

        rejected.open(localUrl(server));
        QTRY_COMPARE_WITH_TIMEOUT(rejected.state(), QAbstractSocket::UnconnectedState, 3000);
        QCOMPARE(server.clientCount(), 1);
        QCOMPARE(server.clientIds().size(), 1); accepted.close(); QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 0, 3000); rejected.abort();
    }

    void broadcastsMessageToEveryConnectedClient()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QSignalSpy connectedSpy(&server, &WebSocketServer::clientConnected);
        QWebSocket first;
        QWebSocket second;
        QWebSocket third;
        QSignalSpy firstMessages(&first, &QWebSocket::textMessageReceived);
        QSignalSpy secondMessages(&second, &QWebSocket::textMessageReceived);
        QSignalSpy thirdMessages(&third, &QWebSocket::textMessageReceived);
        first.open(localUrl(server));
        second.open(localUrl(server));
        third.open(localUrl(server));

        QTRY_COMPARE_WITH_TIMEOUT(connectedSpy.count(), 3, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 3, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(first.state(), QAbstractSocket::ConnectedState, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(second.state(), QAbstractSocket::ConnectedState, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(third.state(), QAbstractSocket::ConnectedState, 3000);

        QString senderId;
        const QString firstPeerPort = QStringLiteral(":%1").arg(first.localPort());
        for (const auto &args : connectedSpy) {
            if (args.at(1).toString().endsWith(firstPeerPort))
                senderId = args.at(0).toString();
        }
        QVERIFY(!senderId.isEmpty());

        first.sendTextMessage(QString::fromUtf8(
            MessageProtocol::encode(MessageProtocol::makeBroadcastRequest(QStringLiteral("hello all")))));

        QTRY_COMPARE_WITH_TIMEOUT(firstMessages.count(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(secondMessages.count(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(thirdMessages.count(), 1, 3000);
        const auto firstEvent = QJsonDocument::fromJson(
            firstMessages.first().first().toString().toUtf8()).object();
        const auto secondEvent = QJsonDocument::fromJson(
            secondMessages.first().first().toString().toUtf8()).object();
        const auto thirdEvent = QJsonDocument::fromJson(
            thirdMessages.first().first().toString().toUtf8()).object();
        QCOMPARE(firstEvent.value(QStringLiteral("type")).toString(), QStringLiteral("broadcast"));
        QCOMPARE(firstEvent.value(QStringLiteral("senderId")).toString(), senderId);
        QCOMPARE(firstEvent.value(QStringLiteral("payload")).toObject()
                     .value(QStringLiteral("text")).toString(), QStringLiteral("hello all"));
        QVERIFY(!firstEvent.value(QStringLiteral("messageId")).toString().isEmpty());
        QVERIFY(!firstEvent.value(QStringLiteral("sentAt")).toString().isEmpty());
        QCOMPARE(secondEvent.value(QStringLiteral("messageId")), firstEvent.value(QStringLiteral("messageId")));
        QCOMPARE(thirdEvent.value(QStringLiteral("messageId")), firstEvent.value(QStringLiteral("messageId")));
    }

    void malformedMessageIsRejectedAndClientCanContinue()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QSignalSpy rejectedSpy(&server, &WebSocketServer::messageRejected);
        QWebSocket client;
        QSignalSpy messagesSpy(&client, &QWebSocket::textMessageReceived);
        client.open(localUrl(server));
        QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::ConnectedState, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 1, 3000);

        client.sendTextMessage(QStringLiteral("{not valid JSON"));
        QTRY_COMPARE_WITH_TIMEOUT(rejectedSpy.count(), 1, 3000);
        QCOMPARE(server.clientCount(), 1);
        QCOMPARE(client.state(), QAbstractSocket::ConnectedState);

        client.sendTextMessage(QString::fromUtf8(
            MessageProtocol::encode(MessageProtocol::makeBroadcastRequest(QStringLiteral("still here")))));
        QTRY_COMPARE_WITH_TIMEOUT(messagesSpy.count(), 1, 3000);
        const auto event = QJsonDocument::fromJson(messagesSpy.first().first().toString().toUtf8()).object();
        QCOMPARE(event.value(QStringLiteral("payload")).toObject()
                     .value(QStringLiteral("text")).toString(), QStringLiteral("still here"));
    }

    void abruptDisconnectIsCleanedAndClientCanReconnect()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QSignalSpy disconnectedSpy(&server, &WebSocketServer::clientDisconnected);
        QWebSocket first;
        first.open(localUrl(server));
        QTRY_COMPARE_WITH_TIMEOUT(first.state(), QAbstractSocket::ConnectedState, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 1, 3000);

        first.abort();
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 0, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(disconnectedSpy.count(), 1, 3000);

        QWebSocket reconnected;
        reconnected.open(localUrl(server));
        QTRY_COMPARE_WITH_TIMEOUT(reconnected.state(), QAbstractSocket::ConnectedState, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 1, 3000);
        reconnected.close();
        QTRY_COMPARE_WITH_TIMEOUT(server.clientCount(), 0, 3000);
    }

    void refusedConnectionReportsErrorWithoutRegisteringClient()
    {
        WebSocketServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QTcpServer unusedPort;
        QVERIFY(unusedPort.listen(QHostAddress::LocalHost, 0));
        const quint16 port = unusedPort.serverPort();
        unusedPort.close();

        QWebSocket client;
        QSignalSpy errorSpy(&client, &QWebSocket::errorOccurred);
        const QUrl unavailableUrl(QStringLiteral("ws://127.0.0.1:%1").arg(port));
        client.open(unavailableUrl);

        QTRY_VERIFY_WITH_TIMEOUT(errorSpy.count() > 0, 3000);
        QCOMPARE(server.clientCount(), 0);
        QVERIFY(client.state() == QAbstractSocket::UnconnectedState ||
                client.state() == QAbstractSocket::ConnectingState);
    }
};

QTEST_GUILESS_MAIN(WebSocketServerTests)
#include "WebSocketServerTests.moc"
