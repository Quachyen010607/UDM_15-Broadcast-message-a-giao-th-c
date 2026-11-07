#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMap>

class ClientListWidget : public QWidget {
    Q_OBJECT
public:
    explicit ClientListWidget(QWidget *parent = nullptr);
    void upsert(const QString &key, const QString &display);
    void removeClient(const QString &key);
    void clearClients();
    int count() const;
private:
    QListWidget *m_list;
    QMap<QString, QListWidgetItem*> m_items;
};

class MessagePanel : public QWidget {
    Q_OBJECT
public:
    explicit MessagePanel(QWidget *parent = nullptr);
    void log(const QString &source, const QString &message);
    void setSendingEnabled(bool value);
    QString message() const;
    void clearMessage();
signals:
    void sendRequested(const QString &message);
private:
    QPlainTextEdit *m_history;
    QLineEdit *m_input;
    QPushButton *m_send;
};

class ProtocolStatusWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProtocolStatusWidget(QWidget *parent = nullptr);
    void setStatus(const QString &protocol, const QString &status, bool active);
private:
    QMap<QString, QLabel*> m_labels;
};
