#include <QMap>
#include <QSet>
#include <QSignalSpy>
#include <QTcpSocket>
#include <QtTest>

#include "../TcpServer.h"

// Client gia lap: ket noi, gui va ghi lai cac message nhan duoc
class TestClient : public QObject
{
public:
    QTcpSocket socket;
    QList<Message> received;

    TestClient()
    {
        connect(&socket, &QTcpSocket::readyRead, this, [this]()
        {
            while (socket.canReadLine())
            {
                received.append(Message::fromJson(socket.readLine().trimmed()));
            }
        });
    }

    int count() const { return int(received.size()); }
    bool isConnected() const { return socket.state() == QAbstractSocket::ConnectedState; }
    bool isUnconnected() const { return socket.state() == QAbstractSocket::UnconnectedState; }
    QString address() const { return "127.0.0.1:" + QString::number(socket.localPort()); }

    void send(const QString& text)
    {
        socket.write(Message::create(text, "Text").toJson() + '\n');
    }
};

class TestTcpMultiClient : public QObject
{
    Q_OBJECT

private:
    TcpServer* m_server = nullptr;
    QList<TestClient*> m_clients;

    TestClient* addClient()
    {
        TestClient* c = new TestClient;
        c->socket.connectToHost(QHostAddress::LocalHost, m_server->port());
        m_clients.append(c);
        return c;
    }

    void connectClients(int n)
    {
        for (int i = 0; i < n; i++)
        {
            addClient();
        }
        QTRY_COMPARE(m_server->clientCount(), n);
        for (TestClient* c : m_clients)
        {
            QTRY_VERIFY(c->isConnected());
        }
    }

private slots:
    void init()
    {
        m_server = new TcpServer;
        m_server->setRelayEnabled(false);
        QVERIFY(m_server->start(0));
    }

    void cleanup()
    {
        qDeleteAll(m_clients);
        m_clients.clear();
        m_server->stop();
        delete m_server;
        m_server = nullptr;
    }

    // 1. Nhieu client ket noi: server dem dung, moi client co dia chi rieng
    void connectEvents()
    {
        QSignalSpy connectedSpy(m_server, &TcpServer::clientConnected);
        connectClients(3);

        QCOMPARE(int(connectedSpy.count()), 3);
        QCOMPARE(int(m_server->clients().size()), 3);

        QSet<QString> addresses;
        for (const ClientInfo& info : m_server->clients())
        {
            addresses.insert(info.peerAddress());
        }
        QCOMPARE(int(addresses.size()), 3);
    }

    // 2. Server broadcast: moi client online deu nhan dung 1 message
    void broadcastReachesAll()
    {
        connectClients(3);

        Message msg = Message::create("hello all", "Alert");
        QList<SendResult> results = m_server->broadcast(msg);

        QCOMPARE(int(results.size()), 3);
        for (const SendResult& r : results)
        {
            QVERIFY(r.success);
        }
        for (TestClient* c : m_clients)
        {
            QTRY_COMPARE(c->count(), 1);
            QCOMPARE(c->received[0].id, msg.id);
            QCOMPARE(c->received[0].content, QString("hello all"));
            QCOMPARE(c->received[0].type, QString("Alert"));
        }
    }

    // 3. Client ngat: server nhan su kien, cac client con lai van nhan tin
    void disconnectEvents()
    {
        connectClients(3);
        QSignalSpy disconnectedSpy(m_server, &TcpServer::clientDisconnected);

        TestClient* leaving = m_clients[1];
        leaving->socket.disconnectFromHost();

        QTRY_COMPARE(int(disconnectedSpy.count()), 1);
        QCOMPARE(m_server->clientCount(), 2);

        QList<SendResult> results = m_server->broadcast(Message::create("after", "Text"));
        QCOMPARE(int(results.size()), 2);
        QTRY_COMPARE(m_clients[0]->count(), 1);
        QTRY_COMPARE(m_clients[2]->count(), 1);
        QCOMPARE(leaving->count(), 0);
    }

    // 4. Client "chet dot ngot" (abort) van duoc phat hien
    void abruptDisconnectIsDetected()
    {
        connectClients(2);
        m_clients[0]->socket.abort();

        QTRY_COMPARE(m_server->clientCount(), 1);
        QList<SendResult> results = m_server->broadcast(Message::create("still here", "Text"));
        QCOMPARE(int(results.size()), 1);
        QVERIFY(results[0].success);
        QTRY_COMPARE(m_clients[1]->count(), 1);
    }

    // 5. Che do chuyen tiep: A gui -> B, C nhan; A khong nhan lai
    void relayBetweenClients()
    {
        connectClients(3);
        m_server->setRelayEnabled(true);

        TestClient* a = m_clients[0];
        TestClient* b = m_clients[1];
        TestClient* c = m_clients[2];

        a->send("tu A");

        QTRY_COMPARE(b->count(), 1);
        QTRY_COMPARE(c->count(), 1);
        QVERIFY(b->received[0].content.endsWith("tu A"));
        QVERIFY(b->received[0].content.contains(a->address()));

        QTest::qWait(200);
        QCOMPARE(a->count(), 0);
    }

    // 6. Thong bao vao/ra cho cac client khac
    void joinLeaveNotices()
    {
        TestClient* a = addClient();
        QTRY_COMPARE(m_server->clientCount(), 1);
        QTRY_VERIFY(a->isConnected());

        m_server->setRelayEnabled(true);

        TestClient* b = addClient();
        QTRY_COMPARE(m_server->clientCount(), 2);
        QTRY_COMPARE(a->count(), 1);
        QCOMPARE(a->received[0].type, QString("Info"));
        QVERIFY(a->received[0].content.contains("da ket noi"));

        b->socket.disconnectFromHost();
        QTRY_COMPARE(a->count(), 2);
        QVERIFY(a->received[1].content.contains("da ngat ket noi"));
        QCOMPARE(b->count(), 0);
    }

    // 7. Tai: 5 client, moi client gui 20 message -> moi client nhan du 80, dung thu tu tung nguoi gui
    void manyClientsStress()
    {
        const int clientCount = 5;
        const int perClient = 20;
        connectClients(clientCount);
        m_server->setRelayEnabled(true);

        for (int i = 0; i < perClient; i++)
        {
            for (TestClient* c : m_clients)
            {
                c->send("seq " + QString::number(i));
            }
        }

        for (TestClient* c : m_clients)
        {
            QTRY_COMPARE_WITH_TIMEOUT(c->count(), (clientCount - 1) * perClient, 5000);

            QMap<QString, int> next;
            for (const Message& m : c->received)
            {
                QString sender = m.content.mid(1, m.content.indexOf(']') - 1);
                int n = m.content.section(' ', -1).toInt();
                QCOMPARE(n, next[sender]);
                next[sender]++;
            }
            QCOMPARE(int(next.size()), clientCount - 1);
            for (int v : next.values())
            {
                QCOMPARE(v, perClient);
            }
        }
    }

    // 8. Server dung: moi client nhan su kien ngat ket noi
    void serverStopDisconnectsClients()
    {
        connectClients(2);
        m_server->stop();

        for (TestClient* c : m_clients)
        {
            QTRY_VERIFY(c->isUnconnected());
        }
        QCOMPARE(m_server->clientCount(), 0);
        QVERIFY(!m_server->isListening());
    }

    // ================= Kiem thu nang cao: dong thoi, du lieu, loi mang =================

    // 9. 50 client ket noi dong thoi, nhan broadcast, roi ngat dong loat
    void fiftyClientsConnectSimultaneously()
    {
        const int n = 50;
        QSignalSpy countSpy(m_server, &TcpServer::clientCountChanged);
        connectClients(n);

        QList<SendResult> results = m_server->broadcast(Message::create("to everyone", "Info"));
        QCOMPARE(int(results.size()), n);
        for (TestClient* c : m_clients)
        {
            QTRY_COMPARE(c->count(), 1);
        }

        for (TestClient* c : m_clients)
        {
            c->socket.disconnectFromHost();
        }
        QTRY_COMPARE_WITH_TIMEOUT(m_server->clientCount(), 0, 5000);
        QCOMPARE(int(countSpy.count()), n * 2);  // 50 lan vao + 50 lan ra
    }

    // 10. Message ~500KB bi TCP cat thanh nhieu manh van duoc ghep lai dung
    void largeMessageIsReassembled()
    {
        connectClients(2);

        QString line = QString::fromUtf16(u"Xin ch\u00e0o th\u1ebf gi\u1edbi \U0001F600 ");
        QString big;
        for (int i = 0; i < 20000; i++)
        {
            big += line + QString::number(i) + "\n";
        }

        m_server->broadcast(Message::create(big, "Text"));

        for (TestClient* c : m_clients)
        {
            QTRY_COMPARE_WITH_TIMEOUT(c->count(), 1, 10000);
            QVERIFY(c->received[0].content == big);
        }
    }

    // 11. Tieng Viet, emoji, dau nhay, gach cheo, xuong dong, tab, dau '|' khong bi hong
    void specialCharactersSurvive()
    {
        connectClients(2);
        QString text = QString::fromUtf16(
            u"Ti\u1ebfng Vi\u1ec7t c\u00f3 d\u1ea5u \"nh\u00e1y\" \\ g\u1ea1ch ng\u01b0\u1ee3c | pipe \U0001F600 \n d\u00f2ng 2\t tab");

        // Server -> client
        m_server->broadcast(Message::create(text, "Text"));
        for (TestClient* c : m_clients)
        {
            QTRY_COMPARE(c->count(), 1);
            QVERIFY(c->received[0].content == text);
        }

        // Client -> server -> client khac (qua che do chuyen tiep)
        m_server->setRelayEnabled(true);
        m_clients[0]->send(text);
        QTRY_COMPARE(m_clients[1]->count(), 2);
        QVERIFY(m_clients[1]->received[1].content.endsWith(text));
    }

    // 12. Du lieu rac, dong trong, message den tung manh: server khong sap, khong ngat client
    void malformedInputDoesNotBreakServer()
    {
        connectClients(2);
        m_server->setRelayEnabled(true);
        TestClient* a = m_clients[0];
        TestClient* b = m_clients[1];

        a->socket.write("\n\n   \n");  // dong trong: bi bo qua
        a->socket.write("khong phai json\n");
        QTRY_COMPARE(b->count(), 1);
        QVERIFY(b->received[0].content.endsWith("khong phai json"));
        QCOMPARE(m_server->clientCount(), 2);

        // Message den thanh 2 manh, giua 2 manh co khoang cho
        a->socket.write("{\"id\":\"x1\",\"content\":\"nua dau");
        a->socket.flush();
        QTest::qWait(200);
        QCOMPARE(b->count(), 1);  // chua du 1 dong -> chua chuyen tiep
        a->socket.write(" nua sau\",\"type\":\"Text\"}\n");
        QTRY_COMPARE(b->count(), 2);
        QVERIFY(b->received[1].content.endsWith("nua dau nua sau"));

        // Server van hoat dong binh thuong
        QCOMPARE(int(m_server->broadcast(Message::create("ok", "Text")).size()), 2);
        QTRY_COMPARE(a->count(), 1);
    }

    // 13. Loi mang: client chet dot ngot trong luc server dang gui lien tuc
    void clientCrashWhileServerIsSending()
    {
        connectClients(3);
        m_clients[0]->socket.abort();

        QString payload(10000, 'x');
        for (int i = 0; i < 100; i++)
        {
            m_server->broadcast(Message::create(payload, "Text"));
        }

        QTRY_COMPARE(m_server->clientCount(), 2);
        QTRY_COMPARE_WITH_TIMEOUT(m_clients[1]->count(), 100, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_clients[2]->count(), 100, 5000);
    }

    // 14. Ket noi / ngat lien tuc 20 lan: khong sot client trong danh sach
    void repeatedConnectDisconnectCycles()
    {
        QSignalSpy connectedSpy(m_server, &TcpServer::clientConnected);
        QSignalSpy disconnectedSpy(m_server, &TcpServer::clientDisconnected);

        for (int i = 0; i < 20; i++)
        {
            TestClient* c = addClient();
            QTRY_COMPARE(m_server->clientCount(), 1);
            QTRY_VERIFY(c->isConnected());
            c->socket.disconnectFromHost();
            QTRY_COMPARE(m_server->clientCount(), 0);
        }
        QCOMPARE(int(connectedSpy.count()), 20);
        QCOMPARE(int(disconnectedSpy.count()), 20);
    }

    // 15. Khong co client nao: broadcast tra ve danh sach rong, khong loi
    void broadcastWithNoClients()
    {
        QList<SendResult> results = m_server->broadcast(Message::create("nobody", "Text"));
        QVERIFY(results.isEmpty());
    }

    // 16. Loi mang: cong dang bi chiem -> start() that bai va bao loi
    void portAlreadyInUse()
    {
        TcpServer second;
        QVERIFY(!second.start(m_server->port()));
        QVERIFY(!second.errorString().isEmpty());
        QVERIFY(!second.isListening());
    }

    // 17. Dung roi khoi dong lai tren cung cong: client moi ket noi duoc
    void serverCanRestart()
    {
        connectClients(2);
        quint16 port = m_server->port();

        m_server->stop();
        QCOMPARE(m_server->clientCount(), 0);
        QVERIFY(m_server->start(port));

        TestClient* c = addClient();
        QTRY_VERIFY(c->isConnected());
        QTRY_COMPARE(m_server->clientCount(), 1);
    }

    // 18. Gui lien tuc tu nhieu client trong luc co client ngat giua chung
    void disconnectDuringRelayTraffic()
    {
        connectClients(4);
        m_server->setRelayEnabled(true);

        for (int i = 0; i < 30; i++)
        {
            m_clients[0]->send("n " + QString::number(i));
            if (i == 10)
            {
                m_clients[3]->socket.abort();  // client 3 roi di giua luc dang truyen
            }
        }

        QTRY_COMPARE(m_server->clientCount(), 3);
        // Client 1 va 2 van nhan du 30 tin cua client 0 (cong them thong bao ngat cua client 3)
        for (int idx = 1; idx <= 2; idx++)
        {
            QTRY_VERIFY_WITH_TIMEOUT(m_clients[idx]->count() >= 30, 5000);
            int fromZero = 0;
            for (const Message& m : m_clients[idx]->received)
            {
                if (m.content.contains(m_clients[0]->address()))
                {
                    fromZero++;
                }
            }
            QCOMPARE(fromZero, 30);
        }
    }
};

QTEST_GUILESS_MAIN(TestTcpMultiClient)
#include "tst_multiclient.moc"
