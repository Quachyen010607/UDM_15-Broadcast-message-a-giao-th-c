#pragma once

#include <QAbstractSocket>
#include <QMainWindow>
#include <QString>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class WebSocketClient;

class WebSocketClientWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit WebSocketClientWindow(QWidget *parent = nullptr);

private:
    void updateConnectionControls(QAbstractSocket::SocketState state);
    void appendLog(const QString &message);
    void sendMessage();

    WebSocketClient *m_client = nullptr;
    QLineEdit *m_urlEdit = nullptr;
    QCheckBox *m_autoReconnectCheck = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPlainTextEdit *m_logEdit = nullptr;
    QLineEdit *m_messageEdit = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPushButton *m_sendButton = nullptr;
};
