#ifndef PLAYBACKSTATE_H
#define PLAYBACKSTATE_H

#include <QMetaType>

enum class PlaybackState
{
    Idle,
    Opening,
    Ready,
    Playing,
    Paused,
    Stopped,
    Seeking,
    Error
};

Q_DECLARE_METATYPE(PlaybackState)

#endif // PLAYBACKSTATE_H
