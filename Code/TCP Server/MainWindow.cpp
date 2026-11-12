#include "MainWindow.h"

#include <QBrush>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("UDM_15 - TCP Server (QTcpServer)");
    resize(900, 620);

    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(5000);

    m_startButton = new QPushButton("Start");
    m_statusLabel = new QLabel;
    m_relayCheck = new QCheckBox("Chuyen tiep message giua cac client");
    m_relayCheck->setChecked(true);

    QHBoxLayout* serverRow = new QHBoxLayout;
    serverRow->addWidget(new QLabel("Port:"));
    serverRow->addWidget(m_portSpin);
    serverRow->addWidget(m_startButton);
    serverRow->addWidget(m_relayCheck);
    serverRow->addWidget(m_statusLabel, 1);

    m_messageEdit = new QLineEdit;
    m_messageEdit->setPlaceholderText("Nhap noi dung message...");
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({ "Text", "Alert", "Info" });
    m_sendButton = new QPushButton("Broadcast");
    m_sendButton->setEnabled(false);

    QHBoxLayout* sendRow = new QHBoxLayout;
    sendRow->addWidget(new QLabel("Noi dung:"));
    sendRow->addWidget(m_messageEdit, 1);
    sendRow->addWidget(m_typeCombo);
    sendRow->addWidget(m_sendButton);

    m_clientTable = new QTableWidget(0, 3);
    m_clientTable->setHorizontalHeaderLabels({ "Protocol", "Client", "Connected at" });
    m_clientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_clientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_clientTable->horizontalHeader()->setStretchLastSection(true);
    m_clientTable->setColumnWidth(1, 160);

    m_resultTable = new QTableWidget(0, 3);
    m_resultTable->setHorizontalHeaderLabels({ "Time", "Client", "Result" });
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->setColumnWidth(1, 160);

    QVBoxLayout* leftBox = new QVBoxLayout;
    leftBox->addWidget(new QLabel("Client dang ket noi"));
    leftBox->addWidget(m_clientTable);

    QVBoxLayout* rightBox = new QVBoxLayout;
    rightBox->addWidget(new QLabel("Ket qua gui"));
    rightBox->addWidget(m_resultTable);

    QHBoxLayout* tables = new QHBoxLayout;
    tables->addLayout(leftBox, 1);
    tables->addLayout(rightBox, 1);

    m_logEdit = new QPlainTextEdit;
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->addLayout(serverRow);
    root->addLayout(sendRow);
    root->addLayout(tables, 1);
    root->addWidget(new QLabel("Log"));
    root->addWidget(m_logEdit);

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartStop);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSend);
    connect(m_messageEdit, &QLineEdit::returnPressed, this, &MainWindow::onSend);
    connect(&m_server, &TcpServer::clientConnected, this, &MainWindow::refreshClients);
    connect(&m_server, &TcpServer::clientDisconnected, this, &MainWindow::refreshClients);
    connect(&m_server, &TcpServer::logMessage, this, &MainWindow::appendLog);
    connect(m_relayCheck, &QCheckBox::toggled, &m_server, &TcpServer::setRelayEnabled);
    m_server.setRelayEnabled(m_relayCheck->isChecked());

    updateStatus();
}

void MainWindow::onStartStop()
{
    if (m_server.isListening())
        m_server.stop();
    else if (!m_server.start(static_cast<quint16>(m_portSpin->value())))
        QMessageBox::warning(this, "UDM_15", "Khong the khoi dong server: " + m_server.errorString());

    refreshClients();
    updateStatus();
}

void MainWindow::onSend()
{
    const QString text = m_messageEdit->text().trimmed();
    if (!m_server.isListening() || text.isEmpty())
        return;

    const Message msg = Message::create(text, m_typeCombo->currentText());
    const QList<SendResult> results = m_server.broadcast(msg);

    if (results.isEmpty())
        addResultRow("-", "Khong co client nao online", false);
    for (const SendResult& result : results)
        addResultRow(result.client, result.success ? "OK" : "FAIL: " + result.error, result.success);

    m_messageEdit->clear();
    m_messageEdit->setFocus();
}

void MainWindow::refreshClients()
{
    m_clientTable->setRowCount(0);
    const QList<ClientInfo> list = m_server.clients();
    for (const ClientInfo& client : list) {
        const int row = m_clientTable->rowCount();
        m_clientTable->insertRow(row);
        m_clientTable->setItem(row, 0, new QTableWidgetItem("TCP"));
        m_clientTable->setItem(row, 1, new QTableWidgetItem(client.peerAddress()));
        m_clientTable->setItem(row, 2, new QTableWidgetItem(
            client.connectedAt().toLocalTime().toString("HH:mm:ss")));
    }
    updateStatus();
}

void MainWindow::appendLog(const QString& text)
{
    m_logEdit->appendPlainText("[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] " + text);
}

void MainWindow::updateStatus()
{
    const bool running = m_server.isListening();
    m_startButton->setText(running ? "Stop" : "Start");
    m_portSpin->setEnabled(!running);
    m_sendButton->setEnabled(running);

    if (running) {
        m_statusLabel->setText("TCP: ON (port " + QString::number(m_server.port()) + ") - " +
                               QString::number(m_server.clientCount()) + " client");
    } else {
        m_statusLabel->setText("TCP: OFF");
    }
}

void MainWindow::addResultRow(const QString& client, const QString& result, bool success)
{
    const int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);

    QTableWidgetItem* timeItem = new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss"));
    QTableWidgetItem* clientItem = new QTableWidgetItem(client);
    QTableWidgetItem* resultItem = new QTableWidgetItem(result);
    resultItem->setForeground(QBrush(success ? Qt::darkGreen : Qt::red));

    m_resultTable->setItem(row, 0, timeItem);
    m_resultTable->setItem(row, 1, clientItem);
    m_resultTable->setItem(row, 2, resultItem);
    m_resultTable->scrollToBottom();
}
