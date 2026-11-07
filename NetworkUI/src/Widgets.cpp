#include "Widgets.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

ClientListWidget::ClientListWidget(QWidget *parent) : QWidget(parent), m_list(new QListWidget(this)) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_list);
}
void ClientListWidget::upsert(const QString &key, const QString &display) {
    if (m_items.contains(key)) m_items.value(key)->setText(display);
    else { auto *item = new QListWidgetItem(display, m_list); m_items.insert(key, item); }
}
void ClientListWidget::removeClient(const QString &key) {
    if (m_items.contains(key)) delete m_items.take(key);
}
void ClientListWidget::clearClients() { m_list->clear(); m_items.clear(); }
int ClientListWidget::count() const { return m_items.size(); }

MessagePanel::MessagePanel(QWidget *parent) : QWidget(parent),
    m_history(new QPlainTextEdit(this)), m_input(new QLineEdit(this)), m_send(new QPushButton("Gửi", this)) {
    auto *v = new QVBoxLayout(this); v->setContentsMargins(0,0,0,0);
    m_history->setReadOnly(true); m_history->setPlaceholderText("Lịch sử tin nhắn và nhật ký...");
    v->addWidget(m_history);
    auto *row = new QHBoxLayout; m_input->setPlaceholderText("Nhập nội dung tin nhắn...");
    row->addWidget(m_input, 1); row->addWidget(m_send); v->addLayout(row);
    connect(m_send, &QPushButton::clicked, this, [this]() {
        const QString msg = m_input->text().trimmed();
        if (!msg.isEmpty()) emit sendRequested(msg);
    });
    connect(m_input, &QLineEdit::returnPressed, m_send, &QPushButton::click);
}
void MessagePanel::log(const QString &source, const QString &message) {
    m_history->appendPlainText(QString("[%1] [%2] %3").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), source, message));
}
void MessagePanel::setSendingEnabled(bool value) { m_input->setEnabled(value); m_send->setEnabled(value); }
QString MessagePanel::message() const { return m_input->text().trimmed(); }
void MessagePanel::clearMessage() { m_input->clear(); }

ProtocolStatusWidget::ProtocolStatusWidget(QWidget *parent) : QWidget(parent) {
    auto *v = new QVBoxLayout(this); v->setContentsMargins(0,0,0,0);
    for (const QString &name : {QString("TCP"),QString("UDP"),QString("WebSocket")}) {
        auto *label = new QLabel(this); label->setWordWrap(true);
        v->addWidget(label); m_labels[name] = label; setStatus(name, "Chưa hoạt động", false);
    }
    v->addStretch();
}
void ProtocolStatusWidget::setStatus(const QString &protocol, const QString &status, bool active) {
    auto *label = m_labels.value(protocol, nullptr);
    if (!label) return;
    label->setText(protocol + ": " + status);
    label->setStyleSheet(active ? "color: #187a42; font-weight: 600;" : "color: #a34a32;");
}
