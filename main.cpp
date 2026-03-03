#include <QApplication>

#include "service/player/playbackstate.h"
#include "view/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<PlaybackState>("PlaybackState");

    MainWindow w;
    w.show();

    return app.exec();
}
