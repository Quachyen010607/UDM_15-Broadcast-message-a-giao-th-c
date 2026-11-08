#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

#include "../TcpClient.h"

// Server gia lap toi gian de kiem thu TcpClient
class FakeServer : public QObject
{
public:
    QTcpServer server;
    QList<QTcpSocket*> sockets;
    QStringList receivedLines;

    FakeServer()
    {
        connect(&server, &QTcpServer::newConnection, this, [this]()
        {
            while (server.hasPendingConnections())
            {
                QTcpSocket* s = server.nextPendingConnection();
                sockets.append(s);
                connect(s, &QTcpSocket::readyRead, this, [this, s]()
                {
                    while (s->canReadLine())
                    {
                        receivedLines.append(QString::fromUtf8(s->readLine().trimmed()));
                    }
                });
                connect(s, &QTcpSocket::disconnected, this, [this, s]()
                {
                    sockets.removeAll(s);
                    s->deleteLater();
                });
            }
        });
    }

    bool start(quint16 port = 0) { return server.listen(QHostAddress::LocalHost, port); }
    quint16 port() const { return server.serverPort(); }
    int socketCount() const { return int(sockets.size()); }
    int lineCount() const { return int(receivedLines.size()); }

    void stop()
    {
        const QList<QTcpSocket*> copy = sockets;
        for (QTcpSocket* s : copy)
        {
            s->abort();
        }
        server.close();
    }

    void send(const Message& msg)
    {
        for (QTcpSocket* s : sockets)
        {
            s->write(msg.toJson() + '\n');
        }
    }
};

class TestTcpClient : public QObject
{
    Q_OBJECT

private:
    FakeServer* m_server = nullptr;
    TcpClient* m_client = nullptr;
    QList<Message> m_got;

    bool isUnconnected() const { return m_client->state() == QAbstractSocket::UnconnectedState; }

    void connectClient()
    {
        m_client->connectToServer("127.0.0.1", m_server->port());
        QTRY_VERIFY(m_client->isConnected());
        QTRY_COMPARE(m_server->socketCount(), 1);
    }

private slots:
    void init()
    {
        m_server = new FakeServer;
        QVERIFY(m_server->start());
        m_client = new TcpClient;
        m_got.clear();
        connect(m_client, &TcpClient::messageReceived, m_client,
                [this](const Message& m, const QString&) { m_got.append(m); });
    }

    void cleanup()
    {
        delete m_client;
        m_client = nullptr;
        m_server->stop();
        delete m_server;
        m_server = nullptr;
    }

    // 1. Su kien ket noi va ngat ket noi chu dong
    void connectAndDisconnectEvents()
    {
        QSignalSpy connectedSpy(m_client, &TcpClient::connected);
        QSignalSpy disconnectedSpy(m_client, &TcpClient::disconnected);

        connectClient();
        QCOMPARE(int(connectedSpy.count()), 1);

        m_client->disconnectFromServer();
        QTRY_VERIFY(isUnconnected());
        QCOMPARE(int(disconnectedSpy.count()), 1);
    }

    // 2. Client gui, server nhan dung noi dung
    void clientSendsToServer()
    {
        connectClient();
        Message msg = Message::create("tu client", "Alert");
        QVERIFY(m_client->sendMessage(msg));

        QTRY_COMPARE(m_server->lineCount(), 1);
        Message received = Message::fromJson(m_server->receivedLines[0].toUtf8());
        QCOMPARE(received.id, msg.id);
        QCOMPARE(received.content, QString("tu client"));
        QCOMPARE(received.type, QString("Alert"));
    }

    // 3. Server gui, client nhan duoc qua signal messageReceived
    void clientReceivesFromServer()
    {
        connectClient();
        Message msg = Message::create("tu server", "Info");
        m_server->send(msg);

        QTRY_COMPARE(int(m_got.size()), 1);
        QCOMPARE(m_got[0].id, msg.id);
        QCOMPARE(m_got[0].content, QString("tu server"));
    }

    // 4. Chua ket noi thi gui that bai (khong crash)
    void sendWhenNotConnectedFails()
    {
        QVERIFY(!m_client->sendMessage(Message::create("x", "Text")));
    }

    // 5. Loi mang: ket noi toi cong khong co ai lang nghe
    void connectToClosedPortFails()
    {
        quint16 deadPort;
        {
            QTcpServer temp;
            QVERIFY(temp.listen(QHostAddress::LocalHost, 0));
            deadPort = temp.serverPort();
        }  // dong ngay -> cong nay khong con ai lang nghe

        QSignalSpy errorSpy(m_client, &TcpClient::errorOccurred);
        QSignalSpy connectedSpy(m_client, &TcpClient::connected);

        m_client->connectToServer("127.0.0.1", deadPort);

        QTRY_VERIFY(errorSpy.count() >= 1);
        QTRY_VERIFY(isUnconnected());
        QVERIFY(!m_client->isConnected());
        QCOMPARE(int(connectedSpy.count()), 0);
    }

    // 6. Loi mang: server tat dot ngot -> client nhan su kien ngat ket noi
    void serverDropsConnection()
    {
        QSignalSpy disconnectedSpy(m_client, &TcpClient::disconnected);
        connectClient();

        m_server->stop();

        QTRY_VERIFY(isUnconnected());
        QCOMPARE(int(disconnectedSpy.count()), 1);
        QVERIFY(!m_client->sendMessage(Message::create("x", "Text")));
    }

    // 7. Message den thanh nhieu manh TCP van duoc ghep lai
    void fragmentedMessageIsReassembled()
    {
        connectClient();
        QTcpSocket* s = m_server->sockets[0];

        s->write("{\"id\":\"f1\",\"content\":\"nua dau");
        s->flush();
        QTest::qWait(200);
        QCOMPARE(int(m_got.size()), 0);

        s->write(" nua sau\",\"type\":\"Text\"}\n");
        QTRY_COMPARE(int(m_got.size()), 1);
        QCOMPARE(m_got[0].content, QString("nua dau nua sau"));
    }

    // 8. Nhieu message dinh lien trong 1 lan gui van tach dung
    void twoMessagesInOneChunk()
    {
        connectClient();
        QByteArray chunk = Message::create("mot", "Text").toJson() + '\n' +
                           Message::create("hai", "Text").toJson() + '\n';
        m_server->sockets[0]->write(chunk);

        QTRY_COMPARE(int(m_got.size()), 2);
        QCOMPARE(m_got[0].content, QString("mot"));
        QCOMPARE(m_got[1].content, QString("hai"));
    }

    // 9. 500 message lien tiep den dung thu tu
    void manyMessagesKeepOrder()
    {
        connectClient();
        for (int i = 0; i < 500; i++)
        {
            m_server->send(Message::create(QString::number(i), "Text"));
        }

        QTRY_COMPARE_WITH_TIMEOUT(int(m_got.size()), 500, 5000);
        for (int i = 0; i < 500; i++)
        {
            QCOMPARE(m_got[i].content, QString::number(i));
        }
    }

    // 10. Tu dong ket noi lai khi server tat roi bat lai tren cung cong
    void autoReconnectAfterServerRestart()
    {
        m_client->setAutoReconnect(true);
        QSignalSpy reconnectSpy(m_client, &TcpClient::reconnectScheduled);
        QSignalSpy connectedSpy(m_client, &TcpClient::connected);

        connectClient();
        quint16 port = m_server->port();

        m_server->stop();
        QTRY_VERIFY(isUnconnected());
        QTRY_VERIFY(reconnectSpy.count() >= 1);

        QVERIFY(m_server->start(port));
        QTRY_VERIFY_WITH_TIMEOUT(m_client->isConnected(), 10000);
        QCOMPARE(int(connectedSpy.count()), 2);

        // Sau khi noi lai van gui nhan binh thuong
        m_server->send(Message::create("sau khi noi lai", "Text"));
        QTRY_COMPARE(int(m_got.size()), 1);
    }

    // 11. Nguoi dung chu dong ngat thi KHONG tu ket noi lai
    void userDisconnectDoesNotReconnect()
    {
        m_client->setAutoReconnect(true);
        QSignalSpy reconnectSpy(m_client, &TcpClient::reconnectScheduled);

        connectClient();
        m_client->disconnectFromServer();
        QTRY_VERIFY(isUnconnected());

        QTest::qWait(3500);  // lau hon chu ky thu lai (3 giay)
        QVERIFY(isUnconnected());
        QCOMPARE(int(reconnectSpy.count()), 0);
    }
};

QTEST_GUILESS_MAIN(TestTcpClient)
#include "tst_tcpclient.moc"
