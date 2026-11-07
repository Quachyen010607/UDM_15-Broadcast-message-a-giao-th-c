#include <QApplication>
#include "ServerWindow.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    app.setStyle("Fusion");
    ServerWindow window;window.resize(1080,650);window.show();
    return app.exec();
}
