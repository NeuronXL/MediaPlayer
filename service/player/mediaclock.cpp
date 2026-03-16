#include "mediaclock.h"

#include <chrono>

MediaClock::MediaClock()
    : m_ptsDrift(0)
{
}

MediaClock::~MediaClock() = default;

int64_t MediaClock::getCurrentTime() {
    int64_t time = nowUs();
    return m_ptsDrift.load(std::memory_order_relaxed) + time;
}

void MediaClock::setCurrentTime(int64_t pts) {
    int64_t time = nowUs();
    m_ptsDrift.store(pts - time, std::memory_order_relaxed);
}

int64_t MediaClock::nowUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
