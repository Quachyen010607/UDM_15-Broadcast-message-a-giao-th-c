#pragma once

#include "Message.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QString>

#include <optional>

namespace udm15 {

// Converts a validated Message into the compact JSON wire representation.
class MessageSerializer final
{
public:
    [[nodiscard]] static std::optional<QByteArray> serialize(
        const Message &message, QString *error = nullptr)
    {
        if (!message.isValid(error))
            return std::nullopt;

        if (error)
            error->clear();

        return QJsonDocument(message.toJson()).toJson(QJsonDocument::Compact);
    }
};

} // namespace udm15
