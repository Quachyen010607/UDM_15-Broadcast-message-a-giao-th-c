#pragma once
#include <QAbstractSocket>
#include <QWidget>

#include "TcpClient.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onConnectClicked();
    void onSend();
    void onConnected();
    void onDisconnected();
    void onStateChanged(QAbstractSocket::SocketState state);
    void onMessageReceived(const Message& msg, const QString& rawLine);
    void onError(const QString& text);
    void onReconnectScheduled(int seconds);

private:
    void appendLog(const QString& text);

    TcpClient m_client;

    QLineEdit* m_hostEdit;
    QSpinBox* m_portSpin;
    QPushButton* m_connectButton;
    QCheckBox* m_reconnectCheck;
    QLabel* m_statusLabel;
    QLineEdit* m_messageEdit;
    QComboBox* m_typeCombo;
    QPushButton* m_sendButton;
    QPlainTextEdit* m_logEdit;
};
