#ifndef PLAYBACKFRAME_H
#define PLAYBACKFRAME_H

#include <QImage>
#include <QMetaType>

#include "../common/timestampeditem.h"

struct PlaybackFrame : public TimestampedItem
{
    QImage image;
    qint64 durationMs = 0;
    bool isKeyFrame = false;
};

Q_DECLARE_METATYPE(PlaybackFrame)

#endif // PLAYBACKFRAME_H
