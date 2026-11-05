#include "WebSocketClientWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    WebSocketClientWindow window;
    window.show();
    return app.exec();
}
