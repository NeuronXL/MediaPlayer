#include "simpleplaybackscheduler.h"

#include "imediaclock.h"

#include <algorithm>

void SimplePlaybackScheduler::setMasterClockType(MasterClockType clockType) {
    m_masterClockType = clockType;
}

int64_t SimplePlaybackScheduler::computeVideoDelayMs(int64_t frameDurationMs,
                                                     IMediaClock& videoClock,
                                                     IMediaClock& audioClock) {
    int64_t delay = std::max<int64_t>(0, frameDurationMs);
    if (m_masterClockType != MasterClockType::Audio) {
        return delay;
    }

    const int64_t diff = videoClock.getCurrentTime() - audioClock.getCurrentTime();
    const int64_t syncThreshold = std::max<int64_t>(40, std::min<int64_t>(100, delay));
    if (diff <= -syncThreshold) {
        delay = std::max<int64_t>(0, delay + diff);
    } else if (diff >= syncThreshold && delay > 100) {
        delay += diff;
    } else if (diff >= syncThreshold) {
        delay = 2 * delay;
    }
    return delay;
}
