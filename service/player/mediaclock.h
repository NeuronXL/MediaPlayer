#ifndef MEDIACLOCK_H
#define MEDIACLOCK_H

#include <atomic>
#include <cstdint>

class MediaClock {
public:
    MediaClock();
    ~MediaClock();

    int64_t getCurrentTime();
    void setCurrentTime(int64_t pts);

private:
    int64_t nowMs();

private:
    std::atomic<int64_t> m_ptsDrift;
};

#endif // MEDIACLOCK_H
