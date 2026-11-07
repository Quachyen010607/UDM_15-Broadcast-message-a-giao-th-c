#include <QtTest/QtTest>
#include <QHostAddress>
#include "Widgets.h"
#include "Protocol.h"
#include "UdpService.h"
class NetworkTests : public QObject {
    Q_OBJECT
private slots:
    void jsonRoundTrip() {
        auto source = Protocol::makeMessage("xin chao", "TCP", "Client");
        QJsonObject parsed;
        QVERIFY(Protocol::decode(Protocol::serializeLine(source),&parsed));
        QCOMPARE(parsed.value("message").toString(),QString("xin chao"));
        QCOMPARE(parsed.value("protocol").toString(),QString("TCP"));
    }
    void jsonInvalid() {
        QVERIFY(!Protocol::decode("not json",nullptr));
        QVERIFY(!Protocol::decode("{\"type\":\"msg\"}",nullptr));
    }
    void clientList() {
        ClientListWidget clients;
        clients.upsert("a","Client A"); clients.upsert("a","Client A changed");
        QCOMPARE(clients.count(),1); clients.removeClient("a"); QCOMPARE(clients.count(),0);
    }
    void udpRejectInvalid() {
        UdpService udp;
        QVERIFY(!udp.startListening(0));
        QVERIFY(!udp.sendMessage("",45455,QHostAddress::LocalHost));
    }
};
QTEST_MAIN(NetworkTests)
#include "test_network_ui.moc"
