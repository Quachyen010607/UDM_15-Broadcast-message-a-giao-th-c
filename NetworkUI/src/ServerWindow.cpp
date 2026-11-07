#include "ServerWindow.h"
#include "ui_ServerWindow.h"
#include "Protocol.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QJsonDocument>

ServerWindow::ServerWindow(QWidget *parent) : QWidget(parent), ui(new Ui::ServerWindow),
    clients(new ClientListWidget(this)), messages(new MessagePanel(this)),
    status(new ProtocolStatusWidget(this))
#ifdef HAVE_WEBSOCKETS
    , ws(QStringLiteral("NetworkUI Server"), QWebSocketServer::NonSecureMode, this)
#endif
{
    ui->setupUi(this);
    auto mount = [](QWidget *host, QWidget *child) {
        auto *v = new QVBoxLayout(host); v->setContentsMargins(0,0,0,0); v->addWidget(child);
    };
    mount(ui->clientHost, clients); mount(ui->messageHost, messages); mount(ui->statusHost, status);
    connect(ui->startButton, &QPushButton::clicked, this, &ServerWindow::start);
    connect(ui->stopButton, &QPushButton::clicked, this, &ServerWindow::stop);
    connect(messages, &MessagePanel::sendRequested, this, &ServerWindow::broadcast);
    connect(&tcp, &QTcpServer::newConnection, this, &ServerWindow::onTcpConnection);
    connect(&udp, &UdpService::messageReceived, this, [this](const QJsonObject &obj, const QString &ip, quint16 port) {
        const QString key = ip + ":" + QString::number(port);
        udpPeers[key] = qMakePair(ip, port);
        clients->upsert("UDP:" + key, "UDP  " + key);
        messages->log("UDP " + key, obj.value("message").toString());
    });
    connect(&udp, &UdpService::invalidDatagram, this, [this](const QString &reason, const QString &ip, quint16 port) {
        messages->log("UDP lỗi", ip + ":" + QString::number(port) + " — " + reason);
    });
    connect(&udp, &UdpService::errorOccurred, this, [this](const QString &error) { messages->log("Lỗi UDP", error); });
#ifdef HAVE_WEBSOCKETS
    connect(&ws, &QWebSocketServer::newConnection, this, [this]() {
        while (ws.hasPendingConnections()) {
            auto *peer = ws.nextPendingConnection(); wsClients.append(peer);
            const QString key = QString::number(reinterpret_cast<quintptr>(peer));
            clients->upsert("WS:" + key, "WebSocket  " + peer->peerAddress().toString());
            messages->log("WebSocket", "Client đã kết nối");
            connect(peer, &QWebSocket::textMessageReceived, this, [this,peer](const QString &raw) {
                QJsonObject obj;
                if (!Protocol::decode(raw.toUtf8(), &obj)) { messages->log("Lỗi WebSocket", "JSON không hợp lệ"); return; }
                messages->log("WebSocket", obj.value("message").toString());
            });
            connect(peer, &QWebSocket::disconnected, this, [this,peer,key]() {
                wsClients.removeAll(peer); clients->removeClient("WS:" + key); peer->deleteLater();
            });
        }
    });
#endif
    ui->stopButton->setEnabled(false); messages->setSendingEnabled(false);
    messages->log("Hệ thống", "Khởi động Server để bắt đầu nhận Client.");
}
ServerWindow::~ServerWindow() { stop(); delete ui; }
void ServerWindow::start() {
    const quint16 port = static_cast<quint16>(ui->serverPort->value());
    if (!tcp.listen(QHostAddress::AnyIPv4, port)) {
        QMessageBox::warning(this, "Lỗi TCP", tcp.errorString());
        messages->log("Lỗi TCP", tcp.errorString());
    } else status->setStatus("TCP", "Lắng nghe cổng " + QString::number(port), true);
    const bool udpOk = udp.startListening(port);
    status->setStatus("UDP", udpOk ? "Lắng nghe cổng " + QString::number(port) : "Không mở được cổng", udpOk);
#ifdef HAVE_WEBSOCKETS
    const quint16 wsPort = static_cast<quint16>(port + (port <= 65533 ? 2 : -2));
    if (ws.listen(QHostAddress::AnyIPv4, wsPort))
        status->setStatus("WebSocket", "Lắng nghe cổng " + QString::number(wsPort), true);
    else { status->setStatus("WebSocket", "Lỗi mở cổng", false); messages->log("Lỗi WebSocket", ws.errorString()); }
#else
    status->setStatus("WebSocket", "Cần cài Qt WebSockets và build lại", false);
#endif
    const bool active = tcp.isListening() || udp.isListening();
#ifdef HAVE_WEBSOCKETS
    ui->stopButton->setEnabled(active || ws.isListening());
#else
    ui->stopButton->setEnabled(active);
#endif
    ui->startButton->setEnabled(false); ui->serverPort->setEnabled(false);
    messages->setSendingEnabled(true);
    messages->log("Server", QString("Đã thử mở TCP/UDP cổng %1").arg(port));
}
void ServerWindow::stop() {
    tcp.close();
    for (auto *s : pending.keys()) { s->disconnectFromHost(); s->deleteLater(); }
    pending.clear(); udp.stopListening(); udpPeers.clear();
#ifdef HAVE_WEBSOCKETS
    ws.close();
    for (auto *s : wsClients) { s->close(); s->deleteLater(); }
    wsClients.clear();
#endif
    clients->clearClients();
    status->setStatus("TCP", "Đã dừng", false); status->setStatus("UDP", "Đã dừng", false);
    status->setStatus("WebSocket", "Đã dừng", false);
    ui->startButton->setEnabled(true); ui->stopButton->setEnabled(false);
    ui->serverPort->setEnabled(true); messages->setSendingEnabled(false);
}
void ServerWindow::onTcpConnection() {
    while (tcp.hasPendingConnections()) {
        QTcpSocket *s = tcp.nextPendingConnection();
        pending.insert(s, QByteArray());
        const auto key = QString::number(reinterpret_cast<quintptr>(s));
        clients->upsert("TCP:" + key, QString("TCP  %1:%2").arg(s->peerAddress().toString()).arg(s->peerPort()));
        messages->log("TCP", "Client đã kết nối");
        connect(s, &QTcpSocket::readyRead, this, [this,s]() { processTcp(s); });
        connect(s, &QTcpSocket::disconnected, this, [this,s,key]() {
            pending.remove(s); clients->removeClient("TCP:" + key);
            messages->log("TCP", "Client ngắt kết nối"); s->deleteLater();
        });
    }
}
void ServerWindow::processTcp(QTcpSocket *s) {
    auto &buf = pending[s]; buf += s->readAll();
    if (buf.size() > 65536) { messages->log("Lỗi TCP", "Buffer quá lớn, ngắt kết nối"); s->disconnectFromHost(); return; }
    while (buf.contains('\n')) {
        const int i = buf.indexOf('\n'); const QByteArray line = buf.left(i); buf.remove(0,i+1);
        QJsonObject obj;
        if (!Protocol::decode(line,&obj)) { messages->log("Lỗi TCP", "JSON không hợp lệ"); continue; }
        messages->log("TCP " + s->peerAddress().toString(), obj.value("message").toString());
    }
}
void ServerWindow::broadcast(const QString &msg) {
    const auto obj = Protocol::makeMessage(msg,"server","Server");
    const auto payload = Protocol::serializeLine(obj);
    int sent = 0;
    for (QTcpSocket *s : pending.keys()) if (s->state() == QAbstractSocket::ConnectedState) {
        if (s->write(payload) >= 0) ++sent;
    }
    for (const auto &peer : udpPeers) {
        QHostAddress address(peer.first);
        if (udp.sendJson(obj, peer.second, address)) ++sent;
    }
#ifdef HAVE_WEBSOCKETS
    for (auto *s : wsClients) if (s->state() == QAbstractSocket::ConnectedState) {
        if (s->sendTextMessage(QString::fromUtf8(payload.trimmed())) >= 0) ++sent;
    }
#endif
    messages->log("Server gửi", msg + QString(" (đã gửi tới %1 endpoint)").arg(sent));
    messages->clearMessage();
}
