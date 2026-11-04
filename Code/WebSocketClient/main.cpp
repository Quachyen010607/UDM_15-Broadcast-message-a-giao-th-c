#include "WebSocketClient.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList arguments = app.arguments();
    const QUrl url(arguments.size() > 1 ? arguments.at(1) : QStringLiteral("ws://127.0.0.1:5002"));
    const QString text = arguments.size() > 2
        ? arguments.mid(2).join(QLatin1Char(' '))
        : QStringLiteral("Hello from Task 3 WebSocket client");

    WebSocketClient client;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &app, [&]() {
        qCritical() << "Timed out waiting for broadcast response";
        client.disconnectFromServer();
        app.exit(1);
    });
    QObject::connect(&client, &WebSocketClient::connected, &app, [&]() {
        qInfo().noquote() << "Connected to" << url.toString();
        if (!client.sendBroadcastText(text))
            app.exit(1);
        else
            timeout.start(8000);
    });
    QObject::connect(&client, &WebSocketClient::broadcastReceived, &app,
                     [&](const QString &senderId, const QString &messageId,
                         const QString &receivedText, const QString &sentAt) {
        qInfo().noquote() << "Broadcast from" << senderId
                          << "(id:" << messageId << ", sentAt:" << sentAt << "):"
                          << receivedText;
        timeout.stop();
        client.disconnectFromServer();
        app.quit();
    });
    QObject::connect(&client, &WebSocketClient::errorOccurred, &app,
                     [](const QString &message) { qWarning().noquote() << message; });
    QObject::connect(&client, &WebSocketClient::disconnected, &app, [&]() {
        if (timeout.isActive()) {
            timeout.stop();
            app.exit(1);
        } else {
            app.quit();
        }
    });

    timeout.start(10000);
    client.connectToServer(url);
    return app.exec();
}
