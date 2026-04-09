#ifndef MEDIACLOCK_H
#define MEDIACLOCK_H

#include "imediaclock.h"

#include <atomic>
#include <cstdint>

class MediaClock : public IMediaClock {
public:
    MediaClock();
    ~MediaClock();

    int64_t getCurrentTime() override;
    void setCurrentTime(int64_t pts) override;

private:
    int64_t nowMs();

private:
    std::atomic<int64_t> m_ptsDrift;
};

#endif // MEDIACLOCK_H
