#include "WebSocketServer.h"

#include <QSignalSpy>
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
        QCOMPARE(server.clientIds().size(), 1);
    }
};

QTEST_GUILESS_MAIN(WebSocketServerTests)
#include "WebSocketServerTests.moc"
