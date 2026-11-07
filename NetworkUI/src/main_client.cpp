#include <QApplication>
#include "ClientWindow.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    app.setStyle("Fusion");
    ClientWindow window;window.resize(1020,630);window.show();
    return app.exec();
}
