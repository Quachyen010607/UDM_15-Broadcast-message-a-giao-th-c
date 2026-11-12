#pragma once

#include "Message.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QString>

#include <optional>

namespace udm15 {

// Decodes an incoming JSON document and validates its shared Message schema.
class MessageParser final
{
public:
    [[nodiscard]] static std::optional<Message> parse(
        const QByteArray &json, QString *error = nullptr)
    {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            setError(error, QStringLiteral("Invalid JSON at byte %1: %2")
                                .arg(parseError.offset)
                                .arg(parseError.errorString()));
            return std::nullopt;
        }

        if (!document.isObject()) {
            setError(error, QStringLiteral("Message JSON root must be an object"));
            return std::nullopt;
        }

        // Message::fromJson enforces required fields, metadata groups and
        // semantic constraints (protocol version, type, non-empty text, UUID).
        return Message::fromJson(document.object(), error);
    }

private:
    static void setError(QString *error, const QString &reason)
    {
        if (error)
            *error = reason;
    }
};

} // namespace udm15
