#include "ClientWindow.h"
#include "ui_ClientWindow.h"
#include "Protocol.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QUrl>

ClientWindow::ClientWindow(QWidget *parent) : QWidget(parent), ui(new Ui::ClientWindow),
    messages(new MessagePanel(this)), status(new ProtocolStatusWidget(this)) {
    ui->setupUi(this);
    auto mount = [](QWidget *host, QWidget *child) {
        auto *v = new QVBoxLayout(host); v->setContentsMargins(0,0,0,0); v->addWidget(child);
    };
    mount(ui->messageHost,messages); mount(ui->statusHost,status);
    connect(ui->connectButton,&QPushButton::clicked,this,&ClientWindow::startConnection);
    connect(ui->disconnectButton,&QPushButton::clicked,this,&ClientWindow::disconnectConnection);
    connect(messages,&MessagePanel::sendRequested,this,&ClientWindow::sendMessage);
    connect(&tcp,&QTcpSocket::connected,this,[this]() {
        status->setStatus("TCP","Đã kết nối",true); messages->log("TCP","Kết nối Server thành công");
        messages->setSendingEnabled(true);
    });
    connect(&tcp,&QTcpSocket::disconnected,this,[this]() {
        status->setStatus("TCP","Đã ngắt kết nối",false);
        messages->setSendingEnabled(false); messages->log("TCP","Server đã ngắt kết nối");
    });
    connect(&tcp,&QTcpSocket::readyRead,this,&ClientWindow::processTcp);
    connect(&tcp,&QTcpSocket::errorOccurred,this,[this](QAbstractSocket::SocketError) {
        messages->log("Lỗi TCP",tcp.errorString());
        status->setStatus("TCP",tcp.errorString(),false);
    });
    connect(&udp,&UdpService::messageReceived,this,[this](const QJsonObject &obj,const QString &ip,quint16 port) {
        messages->log("UDP " + ip + ":" + QString::number(port),obj.value("message").toString());
    });
    connect(&udp,&UdpService::invalidDatagram,this,[this](const QString &error,const QString &ip,quint16) {
        messages->log("UDP không hợp lệ",ip+" - "+error);
    });
    connect(&udp,&UdpService::errorOccurred,this,[this](const QString &error) {messages->log("Lỗi UDP",error);});
#ifdef HAVE_WEBSOCKETS
    connect(&ws,&QWebSocket::connected,this,[this]() {
        messages->log("WebSocket","Đã kết nối Server");
        status->setStatus("WebSocket","Đã kết nối",true); messages->setSendingEnabled(true);
    });
    connect(&ws,&QWebSocket::disconnected,this,[this]() {
        messages->log("WebSocket","Đã ngắt kết nối"); status->setStatus("WebSocket","Đã ngắt kết nối",false);
        messages->setSendingEnabled(false);
    });
    connect(&ws,&QWebSocket::textMessageReceived,this,[this](const QString &raw) {
        QJsonObject obj;
        if (!Protocol::decode(raw.toUtf8(),&obj)) { messages->log("Lỗi WebSocket","JSON không hợp lệ"); return; }
        messages->log("WebSocket nhận",obj.value("message").toString());
    });
    connect(&ws,&QWebSocket::errorOccurred,this,[this](QAbstractSocket::SocketError) {
        messages->log("Lỗi WebSocket",ws.errorString());
    });
#else
    status->setStatus("WebSocket","Chưa cài Qt WebSockets",false);
#endif
    messages->setSendingEnabled(false); ui->disconnectButton->setEnabled(false);
    messages->log("Hướng dẫn", "TCP: cổng 45455; UDP: IP Server và cổng 45455; WebSocket: cổng 45457.");
}
ClientWindow::~ClientWindow() { disconnectConnection(); delete ui; }
void ClientWindow::startConnection() {
    disconnectConnection();
    const QString protocol = ui->protocolBox->currentText();
    const QString host = ui->serverIp->text().trimmed();
    const quint16 port = static_cast<quint16>(ui->serverPort->value());
    if (host.isEmpty()) { QMessageBox::warning(this,"Thiếu IP","Nhập địa chỉ IP Server."); return; }
    if (protocol == "TCP") {
        tcp.connectToHost(host,port);
        messages->log("TCP",QString("Đang kết nối %1:%2...").arg(host).arg(port));
    } else if (protocol == "UDP") {
        // Bind to an ephemeral port; reply from Server goes to this port.
        // Client receives on the next port; server receives on the configured port.
        const quint16 clientPort = port == 65535 ? 45456 : static_cast<quint16>(port + 1);
        if (!udp.startListening(clientPort)) return;
        status->setStatus("UDP",QString("Đang nhận cổng %1").arg(udp.listeningPort()),true);
        messages->setSendingEnabled(true);
        messages->log("UDP","Đã mở cổng nhận. Gửi tin đầu tiên để Server nhận diện client UDP.");
    } else {
#ifdef HAVE_WEBSOCKETS
        QUrl url; url.setScheme("ws"); url.setHost(host); url.setPort(port);
        ws.open(url);
        messages->log("WebSocket","Đang kết nối " + url.toString());
#else
        QMessageBox::warning(this,"Thiếu Qt WebSockets",
                             "Cài gói mingw-w64-ucrt-x86_64-qt6-websockets và build lại để dùng WebSocket.");
        return;
#endif
    }
    ui->connectButton->setEnabled(false); ui->disconnectButton->setEnabled(true);
    ui->protocolBox->setEnabled(false);
}
void ClientWindow::disconnectConnection() {
    if (tcp.state() != QAbstractSocket::UnconnectedState) tcp.abort();
    if (udp.isListening()) udp.stopListening();
#ifdef HAVE_WEBSOCKETS
    if (ws.state() != QAbstractSocket::UnconnectedState) ws.close();
#endif
    status->setStatus("TCP","Chưa kết nối",false);
    status->setStatus("UDP","Chưa mở cổng",false);
    status->setStatus("WebSocket","Chưa kết nối",false);
    messages->setSendingEnabled(false); ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false); ui->protocolBox->setEnabled(true);
}
void ClientWindow::sendMessage(const QString &text) {
    const QString protocol = ui->protocolBox->currentText();
    const auto obj = Protocol::makeMessage(text,protocol,"Client");
    bool ok = false;
    if (protocol == "TCP") {
        ok = tcp.state() == QAbstractSocket::ConnectedState && tcp.write(Protocol::serializeLine(obj)) >= 0;
    } else if (protocol == "UDP") {
        QHostAddress addr;
        if (!addr.setAddress(ui->serverIp->text().trimmed())) {
            messages->log("Lỗi UDP","IP không hợp lệ. UDP yêu cầu IPv4, ví dụ 127.0.0.1.");return;
        }
        ok = udp.sendJson(obj,static_cast<quint16>(ui->serverPort->value()),addr);
    } else {
#ifdef HAVE_WEBSOCKETS
        ok = ws.state() == QAbstractSocket::ConnectedState &&
             ws.sendTextMessage(QString::fromUtf8(Protocol::serializeLine(obj).trimmed())) >= 0;
#endif
    }
    if (ok) { messages->log("Client gửi",text); messages->clearMessage(); }
    else messages->log("Lỗi gửi","Chưa kết nối hoặc gửi thất bại");
}
void ClientWindow::processTcp() {
    buffer += tcp.readAll();
    if (buffer.size() > 65536) {messages->log("Lỗi TCP","Buffer quá lớn");tcp.abort();buffer.clear();return;}
    while (buffer.contains('\n')) {
        const int index = buffer.indexOf('\n'); const QByteArray line = buffer.left(index); buffer.remove(0,index+1);
        QJsonObject obj;
        if (Protocol::decode(line,&obj)) messages->log("TCP nhận",obj.value("message").toString());
        else messages->log("Lỗi TCP","Dữ liệu JSON không hợp lệ");
    }
}
