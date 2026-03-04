#ifndef PLAYBACKFRAME_H
#define PLAYBACKFRAME_H

#include <QImage>
#include <QMetaType>

struct PlaybackFrame
{
    QImage image;
    qint64 ptsMs = 0;
    qint64 durationMs = 0;
    bool isKeyFrame = false;
};

Q_DECLARE_METATYPE(PlaybackFrame)

#endif // PLAYBACKFRAME_H
