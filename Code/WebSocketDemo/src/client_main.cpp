#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDebug>
#include <QWebSocket>

namespace {
constexpr auto DefaultServerUrl = "ws://127.0.0.1:5002";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList arguments = app.arguments();
    const QUrl serverUrl = arguments.size() > 1
        ? QUrl(arguments.at(1))
        : QUrl(QString::fromLatin1(DefaultServerUrl));
    const QString message = arguments.size() > 2
        ? arguments.mid(2).join(QLatin1Char(' '))
        : QStringLiteral("Hello from WebSocket client");

    QWebSocket socket;

    QObject::connect(&socket, &QWebSocket::connected,
                     &app, [&socket, message] {
                         qInfo() << "Connected; sending:" << message;
                         socket.sendTextMessage(message);
                     });

    QObject::connect(&socket, &QWebSocket::textMessageReceived,
                     &app, [&app](const QString &response) {
                         qInfo() << "Received from server:" << response;
                         app.quit();
                     });

    QObject::connect(&socket, &QWebSocket::errorOccurred,
                     &app, [&app, &socket](QAbstractSocket::SocketError) {
                         qWarning().noquote() << "WebSocket error:" << socket.errorString();
                         app.exit(1);
                     });

    QObject::connect(&socket, &QWebSocket::disconnected,
                     &app, [&app] {
                         if (app.closingDown())
                             return;
                         app.quit();
                     });

    qInfo().noquote() << "Connecting to" << serverUrl.toString();
    socket.open(serverUrl);
    return app.exec();
}
