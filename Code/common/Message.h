#pragma once

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
#include <QUuid>

#include <optional>

namespace udm15 {

/*
 * JSON v1 shared by TCP, UDP and WebSocket.
 * Client request: version, type, payload.text.
 * Server event: the same fields plus messageId, senderId and sentAt.
 * TCP framing and UDP datagram handling belong to each transport adapter.
 * Unknown JSON fields are ignored so v1 readers can tolerate extensions.
 */
inline constexpr int MessageProtocolVersion = 1;
inline const QString BroadcastMessageType = QStringLiteral("broadcast");

class Message {
public:
    int version = MessageProtocolVersion;
    QString type = BroadcastMessageType;
    QString messageId;
    QString senderId;
    QDateTime sentAt;
    QString text;

    bool isValid(QString *error = nullptr) const
    {
        const auto reject = [error](const QString &reason) {
            if (error)
                *error = reason;
            return false;
        };

        if (version != MessageProtocolVersion)
            return reject(QStringLiteral("Unsupported message version"));
        if (type != BroadcastMessageType)
            return reject(QStringLiteral("Unsupported message type"));
        if (text.trimmed().isEmpty())
            return reject(QStringLiteral("payload.text must not be empty"));

        const bool hasId = !messageId.isEmpty();
        const bool hasSender = !senderId.isEmpty();
        const bool hasTime = sentAt.isValid();
        if (hasId != hasSender || hasId != hasTime)
            return reject(QStringLiteral("messageId, senderId and sentAt must appear together"));
        if (hasId && QUuid(messageId).isNull())
            return reject(QStringLiteral("messageId must be a UUID"));

        if (error)
            error->clear();
        return true;
    }

    QJsonObject toJson() const
    {
        QJsonObject object{
            {QStringLiteral("version"), version},
            {QStringLiteral("type"), type},
            {QStringLiteral("payload"), QJsonObject{{QStringLiteral("text"), text}}}
        };

        if (!messageId.isEmpty() && !senderId.isEmpty() && sentAt.isValid()) {
            object.insert(QStringLiteral("messageId"), messageId);
            object.insert(QStringLiteral("senderId"), senderId);
            object.insert(QStringLiteral("sentAt"),
                          sentAt.toUTC().toString(Qt::ISODateWithMs));
        }
        return object;
    }

    QByteArray toJsonBytes() const
    {
        return QJsonDocument(toJson()).toJson(QJsonDocument::Compact);
    }

    static std::optional<Message> fromJson(const QJsonObject &object,
                                           QString *error = nullptr)
    {
        const QJsonValue versionValue = object.value(QStringLiteral("version"));
        const QJsonValue typeValue = object.value(QStringLiteral("type"));
        const QJsonValue payloadValue = object.value(QStringLiteral("payload"));
        if (!versionValue.isDouble() || versionValue.toDouble() != MessageProtocolVersion) {
            setError(error, QStringLiteral("version must be 1"));
            return std::nullopt;
        }
        if (!typeValue.isString() || typeValue.toString() != BroadcastMessageType) {
            setError(error, QStringLiteral("type must be broadcast"));
            return std::nullopt;
        }
        if (!payloadValue.isObject() ||
            !payloadValue.toObject().value(QStringLiteral("text")).isString()) {
            setError(error, QStringLiteral("payload.text must be a string"));
            return std::nullopt;
        }

        Message message;
        message.version = versionValue.toInt();
        message.type = typeValue.toString();
        message.text = payloadValue.toObject().value(QStringLiteral("text")).toString();

        const bool hasId = object.contains(QStringLiteral("messageId"));
        const bool hasSender = object.contains(QStringLiteral("senderId"));
        const bool hasTime = object.contains(QStringLiteral("sentAt"));
        if (hasId || hasSender || hasTime) {
            if (!hasId || !hasSender || !hasTime ||
                !object.value(QStringLiteral("messageId")).isString() ||
                !object.value(QStringLiteral("senderId")).isString() ||
                !object.value(QStringLiteral("sentAt")).isString()) {
                setError(error, QStringLiteral("event metadata must include messageId, senderId and sentAt"));
                return std::nullopt;
            }

            message.messageId = object.value(QStringLiteral("messageId")).toString();
            message.senderId = object.value(QStringLiteral("senderId")).toString();
            const QString timestamp = object.value(QStringLiteral("sentAt")).toString();
            message.sentAt = QDateTime::fromString(timestamp, Qt::ISODateWithMs);
            if (!message.sentAt.isValid())
                message.sentAt = QDateTime::fromString(timestamp, Qt::ISODate);
        }

        if (!message.isValid(error))
            return std::nullopt;
        return message;
    }

    static std::optional<Message> decode(const QByteArray &bytes,
                                         QString *error = nullptr)
    {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            setError(error, QStringLiteral("message must be a JSON object"));
            return std::nullopt;
        }
        return fromJson(document.object(), error);
    }

    static Message broadcastRequest(const QString &text)
    {
        Message message;
        message.text = text;
        return message;
    }

    static Message broadcastEvent(const QString &senderId, const QString &text)
    {
        Message message;
        message.messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        message.senderId = senderId;
        message.sentAt = QDateTime::currentDateTimeUtc();
        message.text = text;
        return message;
    }

private:
    static void setError(QString *error, const QString &reason)
    {
        if (error)
            *error = reason;
    }
};

} // namespace udm15
