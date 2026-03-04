#include <QApplication>

#include "service/player/mediainfo.h"
#include "service/player/playbackframe.h"
#include "service/player/playbackstate.h"
#include "view/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<MediaInfo>("MediaInfo");
    qRegisterMetaType<PlaybackFrame>("PlaybackFrame");
    qRegisterMetaType<PlaybackState>("PlaybackState");

    MainWindow w;
    w.show();

    return app.exec();
}
