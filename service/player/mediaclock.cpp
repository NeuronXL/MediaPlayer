#include "mediaclock.h"

#include <chrono>

MediaClock::MediaClock()
    : m_ptsDrift(0)
{
}

MediaClock::~MediaClock() = default;

int64_t MediaClock::getCurrentTime() {
    int64_t time = nowMs();
    return m_ptsDrift.load(std::memory_order_relaxed) + time;
}

void MediaClock::setCurrentTime(int64_t pts) {
    int64_t time = nowMs();
    m_ptsDrift.store(pts - time, std::memory_order_relaxed);
}

int64_t MediaClock::nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
