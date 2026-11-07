#pragma once
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QUuid>
#include <QString>

namespace Protocol {
inline QJsonObject makeMessage(const QString &msg, const QString &via, const QString &sender) {
    return QJsonObject{{"type", "udp_message"}, {"message", msg}, {"protocol", via},
        {"sender", sender}, {"id", QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}};
}
inline QByteArray serializeLine(const QJsonObject &obj) {
    return QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
}
inline bool decode(const QByteArray &bytes, QJsonObject *out) {
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(bytes.trimmed(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;
    auto obj = doc.object();
    if (!obj.value("message").isString() || obj.value("message").toString().trimmed().isEmpty()) return false;
    if (out) *out = obj;
    return true;
}
}
