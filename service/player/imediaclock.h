#ifndef IMEDIACLOCK_H
#define IMEDIACLOCK_H

#include <cstdint>

class IMediaClock {
public:
    virtual ~IMediaClock() = default;

    virtual int64_t getCurrentTime() = 0;
    virtual void setCurrentTime(int64_t ptsMs) = 0;
};

#endif // IMEDIACLOCK_H
