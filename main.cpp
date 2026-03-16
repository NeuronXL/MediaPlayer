#include <QApplication>

#include "service/player/playstate.h"
#include "view/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<PlayState>("PlayState");

    MainWindow w;
    w.show();

    return app.exec();
}
