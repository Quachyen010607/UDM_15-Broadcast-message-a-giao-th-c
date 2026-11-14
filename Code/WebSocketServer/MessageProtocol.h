#pragma once

#include "../common/Message.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace MessageProtocol {
constexpr int Version = udm15::MessageProtocolVersion;

// Compatibility helpers for the WebSocket module; serialization and validation
// are delegated to the shared transport-independent Message model.
inline QJsonObject makeBroadcastRequest(const QString &text)
{
    return udm15::Message::broadcastRequest(text).toJson();
}

inline QJsonObject makeBroadcastEvent(const QString &senderId, const QString &text)
{
    return udm15::Message::broadcastEvent(senderId, text).toJson();
}

inline QByteArray encode(const QJsonObject &message)
{
    return QJsonDocument(message).toJson(QJsonDocument::Compact);
}

inline bool parseBroadcastRequest(const QString &wireText, QString *text, QString *error)
{
    const auto message = udm15::Message::decode(wireText.toUtf8(), error);
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
