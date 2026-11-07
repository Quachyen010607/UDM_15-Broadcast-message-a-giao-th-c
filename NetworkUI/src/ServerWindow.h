#pragma once
#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QHostAddress>
#include "UdpService.h"
#include "Widgets.h"
#ifdef HAVE_WEBSOCKETS
#include <QWebSocketServer>
#include <QWebSocket>
#endif
namespace Ui { class ServerWindow; }
class ServerWindow : public QWidget {
    Q_OBJECT
public:
    explicit ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow() override;
private:
    Ui::ServerWindow *ui;
    ClientListWidget *clients;
    MessagePanel *messages;
    ProtocolStatusWidget *status;
    QTcpServer tcp;
    UdpService udp;
    QHash<QTcpSocket*, QByteArray> pending;
    QHash<QString, QPair<QString, quint16>> udpPeers;
#ifdef HAVE_WEBSOCKETS
    QWebSocketServer ws;
    QList<QWebSocket*> wsClients;
#endif
    void start();
    void stop();
    void broadcast(const QString &msg);
    void onTcpConnection();
    void processTcp(QTcpSocket *socket);
};
