#include "WebSocketServer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>

namespace {
constexpr quint16 DefaultWebSocketPort = 5002;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("UDM_15 WebSocket Server"));

    quint16 port = DefaultWebSocketPort;
    const QStringList arguments = app.arguments();
    if (arguments.size() > 1) {
        bool ok = false;
        const uint requestedPort = arguments.at(1).toUInt(&ok);
        if (!ok || requestedPort == 0 || requestedPort > 65535) {
            qCritical() << "Usage:" << arguments.first() << "[port: 1..65535]";
            return 2;
        }
        port = static_cast<quint16>(requestedPort);
    }

    WebSocketServer server;
    QObject::connect(&server, &WebSocketServer::clientConnected,
                     [](const QString &peer) {
                         qInfo().noquote() << "Client connected:" << peer;
                     });
    QObject::connect(&server, &WebSocketServer::clientDisconnected,
                     [](const QString &peer) {
                         qInfo().noquote() << "Client disconnected:" << peer;
                     });
    QObject::connect(&server, &WebSocketServer::textMessageReceived,
                     [](const QString &peer, const QString &message) {
                         qInfo().noquote() << "Message from" << peer << ':' << message;
                     });
    QObject::connect(&server, &WebSocketServer::serverError,
                     [](const QString &message) {
                         qWarning().noquote() << "WebSocket error:" << message;
                     });

    if (!server.listen(QHostAddress::AnyIPv4, port))
        return 1;

    qInfo() << "WebSocket server listening on port" << server.serverPort();
    return app.exec();
}
