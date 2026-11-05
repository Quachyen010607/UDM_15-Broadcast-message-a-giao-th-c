#include "UdpService.h"

#include <QCoreApplication>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTest>
#include <QUdpSocket>

class TestUdpService : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        qRegisterMetaType<QJsonObject>("QJsonObject");
    }

    void invalidPortsAndMessages() {
        UdpService udp;
        QSignalSpy errors(&udp, &UdpService::errorOccurred);
        QVERIFY(!udp.startListening(0));
        QVERIFY(!udp.sendMessage("test", 0));
        QVERIFY(!udp.sendMessage("  ", 45454));
        QVERIFY(!udp.sendJson(QJsonObject{{"x", 1}}, 0));
        QVERIFY(errors.count() >= 4);
    }

    void sendAndReceiveLocalhost() {
        UdpService receiver;
        // Reserve a free ephemeral port via Qt's UDP bind, then let service bind it.
        QUdpSocket temporary;
        QVERIFY(temporary.bind(QHostAddress::LocalHost, 0));
        const quint16 port = temporary.localPort();
        temporary.close();
        QVERIFY(receiver.startListening(port));
        QCOMPARE(receiver.listeningPort(), port);
        QVERIFY(receiver.isListening());

        QSignalSpy received(&receiver, &UdpService::messageReceived);
        UdpService sender;
        QVERIFY(sender.sendMessage("Xin chao UDP", port, QHostAddress::LocalHost));
        QTRY_COMPARE_WITH_TIMEOUT(received.count(), 1, 3000);
        const QList<QVariant> arguments = received.takeFirst();
        const QJsonObject obj = qvariant_cast<QJsonObject>(arguments.at(0));
        QCOMPARE(obj.value("type").toString(), QString("udp_message"));
        QCOMPARE(obj.value("message").toString(), QString("Xin chao UDP"));
        QVERIFY(!obj.value("id").toString().isEmpty());
        QVERIFY(!obj.value("timestamp").toString().isEmpty());

        receiver.stopListening();
        QVERIFY(!receiver.isListening());
    }

    void invalidDatagram() {
        UdpService receiver;
        QUdpSocket temporary;
        QVERIFY(temporary.bind(QHostAddress::LocalHost, 0));
        const quint16 port = temporary.localPort();
        temporary.close();
        QVERIFY(receiver.startListening(port));
        QSignalSpy invalid(&receiver, &UdpService::invalidDatagram);

        QUdpSocket sender;
        const QByteArray malformed = "not-json";
        QCOMPARE(sender.writeDatagram(malformed, QHostAddress::LocalHost, port), qint64(malformed.size()));
        QTRY_COMPARE_WITH_TIMEOUT(invalid.count(), 1, 3000);

        const QByteArray wrongSchema = R"({"type":"other","message":"hello"})";
        QCOMPARE(sender.writeDatagram(wrongSchema, QHostAddress::LocalHost, port), qint64(wrongSchema.size()));
        QTRY_COMPARE_WITH_TIMEOUT(invalid.count(), 2, 3000);
    }

    void tooLargePayload() {
        UdpService sender;
        QSignalSpy errors(&sender, &UdpService::errorOccurred);
        QJsonObject huge;
        huge["message"] = QString(2000, QLatin1Char('a'));
        QVERIFY(!sender.sendJson(huge, 45454, QHostAddress::LocalHost));
        QCOMPARE(errors.count(), 1);
    }
};
QTEST_GUILESS_MAIN(TestUdpService)
#include "test_udp.moc"
