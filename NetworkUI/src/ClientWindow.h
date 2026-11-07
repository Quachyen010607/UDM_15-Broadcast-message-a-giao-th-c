#pragma once
#include <QWidget>
#include <QTcpSocket>
#include "UdpService.h"
#include "Widgets.h"
#ifdef HAVE_WEBSOCKETS
#include <QWebSocket>
#endif
namespace Ui { class ClientWindow; }
class ClientWindow : public QWidget {
    Q_OBJECT
public:
    explicit ClientWindow(QWidget *parent = nullptr);
    ~ClientWindow() override;
private:
    Ui::ClientWindow *ui;
    MessagePanel *messages;
    ProtocolStatusWidget *status;
    QTcpSocket tcp;
    UdpService udp;
    QByteArray buffer;
#ifdef HAVE_WEBSOCKETS
    QWebSocket ws;
#endif
    void startConnection();
    void disconnectConnection();
    void sendMessage(const QString &text);
    void processTcp();
};
