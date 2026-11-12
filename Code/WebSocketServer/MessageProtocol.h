#pragma once

#include "../common/Message.h"
#include "../common/MessageSerializer.h"

#include <QJsonObject>
#include <QString>

namespace MessageProtocol {
constexpr int Version = udm15::MessageProtocolVersion;

// Compatibility helpers for the WebSocket module; wire encoding is delegated
// to the shared MessageSerializer.
inline QJsonObject makeBroadcastRequest(const QString &text)
{
    return udm15::Message::broadcastRequest(text).toJson();
}

inline QJsonObject makeBroadcastEvent(const QString &senderId, const QString &text)
{
    return udm15::Message::broadcastEvent(senderId, text).toJson();
}

inline QByteArray encode(const QJsonObject &object)
{
    const auto message = udm15::Message::fromJson(object);
    if (!message)
        return {};

    const auto bytes = udm15::MessageSerializer::serialize(*message);
    return bytes.value_or(QByteArray{});
}

inline bool parseBroadcastRequest(const QString &wireText, QString *text, QString *error)
{
    const auto message = udm15::MessageSerializer::deserialize(wireText.toUtf8(), error);
    if (!message)
        return false;

    // Requests from clients must not carry server-generated event metadata.
    if (!message->messageId.isEmpty() || !message->senderId.isEmpty() || message->sentAt.isValid()) {
        if (error)
            *error = QStringLiteral("Client request must not contain event metadata");
        return false;
    }

    if (text)
        *text = message->text;
    if (error)
        error->clear();
    return true;
}
} // namespace MessageProtocol
