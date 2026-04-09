#ifndef SIMPLEPLAYBACKSCHEDULER_H
#define SIMPLEPLAYBACKSCHEDULER_H

#include "iplaybackscheduler.h"

class SimplePlaybackScheduler : public IPlaybackScheduler {
public:
    void setMasterClockType(MasterClockType clockType) override;
    int64_t computeVideoDelayMs(int64_t frameDurationMs,
                                IMediaClock& videoClock,
                                IMediaClock& audioClock) override;

private:
    MasterClockType m_masterClockType = MasterClockType::Audio;
};

#endif // SIMPLEPLAYBACKSCHEDULER_H
