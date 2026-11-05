#include "UdpService.h"

#include <QApplication>
#include <QFormLayout>
#include <QHostAddress>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("UDP Broadcast - Qt Demo");
    window.resize(720, 460);

    UdpService udp;
    auto *layout = new QVBoxLayout(&window);
    auto *form = new QFormLayout();
    auto *receivePort = new QLineEdit("45454");
    auto *destinationPort = new QLineEdit("45454");
    auto *destinationIp = new QLineEdit("255.255.255.255");
    auto *message = new QLineEdit();
    auto *log = new QTextEdit();
    log->setReadOnly(true);
    auto *openBtn = new QPushButton("Mở cổng nhận");
    auto *closeBtn = new QPushButton("Dừng nhận");
    auto *sendBtn = new QPushButton("Gửi UDP");
    auto *validator = new QIntValidator(1, 65535, &window);
    receivePort->setValidator(validator);
    destinationPort->setValidator(validator);
    form->addRow("Cổng nhận:", receivePort);
    form->addRow("IP đích/broadcast:", destinationIp);
    form->addRow("Cổng đích:", destinationPort);
    form->addRow("Tin nhắn:", message);
    layout->addLayout(form);
    auto *buttons = new QHBoxLayout();
    buttons->addWidget(openBtn);
    buttons->addWidget(closeBtn);
    buttons->addWidget(sendBtn);
    layout->addLayout(buttons);
    layout->addWidget(new QLabel("Nhật ký gửi/nhận:"));
    layout->addWidget(log);

    QObject::connect(openBtn, &QPushButton::clicked, [&]() {
        if (!receivePort->hasAcceptableInput()) { log->append("[LỖI] Cổng nhận không hợp lệ"); return; }
        udp.startListening(receivePort->text().toUShort());
    });
    QObject::connect(closeBtn, &QPushButton::clicked, [&]() { udp.stopListening(); });
    QObject::connect(sendBtn, &QPushButton::clicked, [&]() {
        if (!destinationPort->hasAcceptableInput()) { log->append("[LỖI] Cổng đích không hợp lệ"); return; }
        QHostAddress address;
        if (!address.setAddress(destinationIp->text().trimmed()) ||
            address.protocol() != QAbstractSocket::IPv4Protocol) {
            log->append("[LỖI] IP đích phải là địa chỉ IPv4 hợp lệ");
            return;
        }
        udp.sendMessage(message->text(), destinationPort->text().toUShort(), address);
    });
    QObject::connect(message, &QLineEdit::returnPressed, sendBtn, &QPushButton::click);
    QObject::connect(&udp, &UdpService::messageReceived, [&](const QJsonObject &obj, const QString &ip, quint16 port) {
        log->append(QString("[NHẬN] %1:%2 → %3").arg(ip).arg(port).arg(obj.value("message").toString().toHtmlEscaped()));
    });
    QObject::connect(&udp, &UdpService::invalidDatagram, [&](const QString &reason, const QString &ip, quint16 port) {
        log->append(QString("[GÓI LỖI] %1:%2 — %3").arg(ip).arg(port).arg(reason.toHtmlEscaped()));
    });
    QObject::connect(&udp, &UdpService::statusChanged, [&](const QString &s) { log->append("[TRẠNG THÁI] " + s.toHtmlEscaped()); });
    QObject::connect(&udp, &UdpService::errorOccurred, [&](const QString &s) { log->append("[LỖI] " + s.toHtmlEscaped()); });

    udp.startListening(45454);
    window.show();
    return app.exec();
}
