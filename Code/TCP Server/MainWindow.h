#pragma once
#include <QWidget>

#include "TcpServer.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onStartStop();
    void onSend();
    void refreshClients();
    void appendLog(const QString& text);

private:
    void updateStatus();
    void addResultRow(const QString& client, const QString& result, bool success);

    TcpServer m_server;

    QSpinBox* m_portSpin;
    QPushButton* m_startButton;
    QLabel* m_statusLabel;
    QCheckBox* m_relayCheck;
    QLineEdit* m_messageEdit;
    QComboBox* m_typeCombo;
    QPushButton* m_sendButton;
    QTableWidget* m_clientTable;
    QTableWidget* m_resultTable;
    QPlainTextEdit* m_logEdit;
};
