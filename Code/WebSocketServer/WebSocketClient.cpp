#include "MessageProtocol.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList arguments = app.arguments();
    const QUrl url(arguments.size() > 1 ? arguments.at(1) : QStringLiteral("ws://127.0.0.1:5002"));
    if (!url.isValid() || (url.scheme() != QStringLiteral("ws") && url.scheme() != QStringLiteral("wss"))) {
        qCritical() << "Usage: udm15_websocket_client [ws://host:port] [message text]";
        return 2;
    }

    const QString text = arguments.size() > 2
        ? arguments.mid(2).join(QLatin1Char(' '))
        : QStringLiteral("Hello from the Qt WebSocket client");
    QWebSocket socket;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &app, [&]() {
        qCritical().noquote() << "No broadcast received:" << socket.errorString();
        socket.abort();
        app.exit(1);
    });
    QObject::connect(&socket, &QWebSocket::connected, &app, [&]() {
        const QByteArray request = MessageProtocol::encode(MessageProtocol::makeBroadcastRequest(text));
        socket.sendTextMessage(QString::fromUtf8(request));
        timeout.start(8000);
    });
    QObject::connect(&socket, &QWebSocket::textMessageReceived, &app,
                     [&](const QString &wireText) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(wireText.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            qCritical() << "Server returned an invalid JSON message";
            app.exit(1);
            return;
        }
        qInfo().noquote() << "Broadcast received:" << document.toJson(QJsonDocument::Compact);
        timeout.stop();
        app.quit();
    });
    QObject::connect(&socket, &QWebSocket::disconnected, &app, [&]() {
        if (timeout.isActive()) {
            qCritical() << "Server disconnected before replying";
            app.exit(1);
        }
    });

    timeout.start(8000);
    socket.open(url);
    return app.exec();
}
