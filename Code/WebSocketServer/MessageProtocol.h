#pragma once

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
#include <QUuid>

namespace MessageProtocol {
constexpr int Version = 1;

inline QJsonObject makeBroadcastRequest(const QString &text)
{
    return {{QStringLiteral("version"), Version},
            {QStringLiteral("type"), QStringLiteral("broadcast")},
            {QStringLiteral("payload"), QJsonObject{{QStringLiteral("text"), text}}}};
}

inline QJsonObject makeBroadcastEvent(const QString &senderId, const QString &text)
{
    return {{QStringLiteral("version"), Version},
            {QStringLiteral("type"), QStringLiteral("broadcast")},
            {QStringLiteral("messageId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {QStringLiteral("senderId"), senderId},
            {QStringLiteral("sentAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
            {QStringLiteral("payload"), QJsonObject{{QStringLiteral("text"), text}}}};
}

inline QByteArray encode(const QJsonObject &message)
{
    return QJsonDocument(message).toJson(QJsonDocument::Compact);
}

inline bool parseBroadcastRequest(const QString &wireText, QString *text, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(wireText.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = QStringLiteral("Message must be a JSON object");
        return false;
    }

    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("version")).toInt(-1) != Version ||
        object.value(QStringLiteral("type")).toString() != QStringLiteral("broadcast")) {
        if (error) *error = QStringLiteral("Expected protocol v1 broadcast message");
        return false;
    }

    const QJsonValue payloadValue = object.value(QStringLiteral("payload"));
    if (!payloadValue.isObject() ||
        !payloadValue.toObject().value(QStringLiteral("text")).isString()) {
        if (error) *error = QStringLiteral("payload.text must be a string");
        return false;
    }

    if (text)
        *text = payloadValue.toObject().value(QStringLiteral("text")).toString();
    return true;
}
} // namespace MessageProtocol
