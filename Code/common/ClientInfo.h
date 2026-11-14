#pragma once

#include <QDateTime>
#include <QString>
#include <utility>

class ClientInfo
{
public:
    ClientInfo() = default;

    ClientInfo(QString clientId, QString peerAddress,
               QDateTime connectedAt = QDateTime::currentDateTimeUtc())
        : m_clientId(std::move(clientId)),
          m_peerAddress(std::move(peerAddress)),
          m_connectedAt(std::move(connectedAt))
    {
    }

    const QString &clientId() const { return m_clientId; }
    const QString &peerAddress() const { return m_peerAddress; }
    const QDateTime &connectedAt() const { return m_connectedAt; }

    bool isValid() const
    {
        return !m_clientId.trimmed().isEmpty()
            && !m_peerAddress.trimmed().isEmpty()
            && m_connectedAt.isValid();
    }

private:
    QString m_clientId;
    QString m_peerAddress;
    QDateTime m_connectedAt;
};
