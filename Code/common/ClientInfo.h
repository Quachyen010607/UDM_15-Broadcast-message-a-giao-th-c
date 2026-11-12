#pragma once

#include <QDateTime>
#include <QString>
#include <utility>

namespace udm15 {

// Value object describing one remote client managed by a transport server.
class ClientInfo final
{
public:
    ClientInfo() = default;

    ClientInfo(QString id, QString peer, QDateTime connectedAt = QDateTime::currentDateTimeUtc())
        : m_id(std::move(id)),
          m_peer(std::move(peer)),
          m_connectedAt(connectedAt.toUTC())
    {
    }

    const QString &id() const { return m_id; }
    const QString &peer() const { return m_peer; }
    const QDateTime &connectedAt() const { return m_connectedAt; }

    bool isValid() const
    {
        return !m_id.trimmed().isEmpty() &&
               !m_peer.trimmed().isEmpty() &&
               m_connectedAt.isValid();
    }

private:
    QString m_id;
    QString m_peer;
    QDateTime m_connectedAt;
};

} // namespace udm15
