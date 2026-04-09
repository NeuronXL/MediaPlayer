#ifndef IPLAYBACKSCHEDULER_H
#define IPLAYBACKSCHEDULER_H

#include "masterclocktype.h"

#include <cstdint>

class IMediaClock;

class IPlaybackScheduler {
public:
    virtual ~IPlaybackScheduler() = default;

    virtual void setMasterClockType(MasterClockType clockType) = 0;
    virtual int64_t computeVideoDelayMs(int64_t frameDurationMs,
                                        IMediaClock& videoClock,
                                        IMediaClock& audioClock) = 0;
};

#endif // IPLAYBACKSCHEDULER_H
