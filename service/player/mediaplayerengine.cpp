#include "mediaplayerengine.h"

#include "../logging/logservice.h"
#include "../pipeline/mediapipelineservice.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

MediaPlayerEngine::MediaPlayerEngine(std::shared_ptr<IVideoAdapter> videoAdapter,
                                     std::shared_ptr<IAudioAdapter> audioAdapter)
    : m_pipelineService(new MediaPipelineService())
    , m_audioAdapter(std::move(audioAdapter))
    , m_nextSubscriptionId(1)
    , m_subscriberMutex()
    , m_subscribers()
    , m_masterClockType(MasterClockType::Audio)
    , m_videoClock()
    , m_audioClock()
    , m_playState(PlayState::Paused)
    , m_filePath()
    , m_curPts(0) {
    (void)videoAdapter;
    m_videoFeedThread = std::thread(&MediaPlayerEngine::videoFeed, this);
    m_audioFeedThread = std::thread(&MediaPlayerEngine::audioFeed, this);
}

MediaPlayerEngine::SubscriptionId MediaPlayerEngine::subscribe(EngineEventMask eventMask, EventHandler handler,
                                                               EventExecutor executor) {
    if (!handler || eventMask == 0) {
        return 0;
    }

    Subscriber subscriber;
    subscriber.id = m_nextSubscriptionId.fetch_add(1, std::memory_order_relaxed);
    subscriber.eventMask = eventMask;
    subscriber.handler = std::move(handler);
    subscriber.executor = std::move(executor);

    std::lock_guard<std::mutex> lock(m_subscriberMutex);
    m_subscribers.push_back(std::move(subscriber));
    return m_subscribers.back().id;
}

void MediaPlayerEngine::unsubscribe(SubscriptionId subscriptionId) {
    if (subscriptionId == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_subscriberMutex);
    m_subscribers.erase(
        std::remove_if(m_subscribers.begin(), m_subscribers.end(),
                       [subscriptionId](const Subscriber& subscriber) { return subscriber.id == subscriptionId; }),
        m_subscribers.end());
}

void MediaPlayerEngine::publishEvent(const EngineEvent& event) {
    std::vector<Subscriber> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_subscriberMutex);
        snapshot = m_subscribers;
    }

    const EngineEventMask mask = eventMaskOf(event);
    for (const auto& subscriber : snapshot) {
        if ((subscriber.eventMask & mask) == 0 || !subscriber.handler) {
            continue;
        }

        if (subscriber.executor) {
            auto copiedEvent = event;
            auto handler = subscriber.handler;
            subscriber.executor([handler = std::move(handler), copiedEvent = std::move(copiedEvent)]() mutable {
                handler(copiedEvent);
            });
        } else {
            subscriber.handler(event);
        }
    }
}

MediaPlayerEngine::~MediaPlayerEngine() {
    if (m_videoFeedThread.joinable()) {
        if (m_videoFeedThread.get_id() != std::this_thread::get_id()) {
            m_videoFeedThread.join();
        } else {
            m_videoFeedThread.detach();
        }
    }
    if (m_audioFeedThread.joinable()) {
        if (m_audioFeedThread.get_id() != std::this_thread::get_id()) {
            m_audioFeedThread.join();
        } else {
            m_audioFeedThread.detach();
        }
    }
}

void MediaPlayerEngine::play() {
    m_playState = PlayState::Playing;
    if (m_audioAdapter) {
        m_audioAdapter->start();
    }
}
void MediaPlayerEngine::pause() {
}

void MediaPlayerEngine::openMedia(std::string filePath) {
    m_filePath = filePath;
    MediaSourceInfo sourceInfo;
    std::string errorMessage;
    const bool opened = m_pipelineService->openMedia(filePath, &sourceInfo, &errorMessage);
    m_videoClock.setCurrentTime(0);
    m_audioClock.setCurrentTime(0);

    if (opened) {
        publishEvent(OpenMediaSucceededEvent{sourceInfo});
    } else {
        publishEvent(OpenMediaFailedEvent{m_filePath, errorMessage});
    }
}

void MediaPlayerEngine::videoFeed() {
    int64_t delay = 10;
    while (true) {
        if (m_playState == PlayState::Paused) {
            delay = 10;
        } else if (m_playState == PlayState::Playing) {
            FramePtr nextFrame = m_pipelineService->peekNextVideoFrame();
            if (!nextFrame) {
                goto nextVideoLoop;
            }

            m_videoClock.setCurrentTime(nextFrame->pts);
            if (auto videoFrame = std::dynamic_pointer_cast<VideoFrame>(nextFrame)) {
                publishEvent(VideoFrameEvent{videoFrame});
            }
            delay = computeClockDelay(nextFrame->duration);
            auto droppedFrame = m_pipelineService->popVideoFrame();
            (void)droppedFrame;
        }
nextVideoLoop:
        std::this_thread::sleep_for(std::chrono::milliseconds(std::max(delay, 0ll)));
    }
}

void MediaPlayerEngine::audioFeed() {
    int64_t delay = 100;
    while (true) {
        if (m_playState == PlayState::Paused) {
            if (m_audioAdapter) {
                m_audioAdapter->pause();
            }
        } else if (m_playState == PlayState::Playing) {
            if (m_audioAdapter) {
                m_audioAdapter->start();
            }
            int64_t curTime = m_audioAdapter ? m_audioAdapter->playedTimeMs() : 0;
            FramePtr nextFrame = m_pipelineService->peekNextAudioFrame();
            if (!nextFrame) {
                delay = 5;
                goto nextAudioLoop;
            }
            int64_t pcmLength = nextFrame->pts - curTime;
            m_audioClock.setCurrentTime(curTime);
            if (pcmLength + delay < 200) {
                delay = 5;
            } else {
                delay = 100;
            }
            if (nextFrame) {
                if (auto audioFrame =
                        std::dynamic_pointer_cast<AudioFrame>(nextFrame)) {
                    if (m_audioAdapter) {
                        m_audioAdapter->write(audioFrame);
                    }
                }
                auto consumedFrame = m_pipelineService->popAudioFrame();
                (void)consumedFrame;
            }
        }
nextAudioLoop:
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}

int64_t MediaPlayerEngine::computeClockDelay(int64_t delay) {
    int64_t diff;
    int64_t syncThreshold = 0;
    if (m_masterClockType == MasterClockType::Audio) {
        diff = m_videoClock.getCurrentTime() - m_audioClock.getCurrentTime();
        syncThreshold = std::max(1ll * 40, std::min(1ll * 100, delay));
        if (diff <= -syncThreshold) {
            delay = std::max(0ll, delay + diff);
        } else if (diff >= syncThreshold && delay > 100ll) {
            delay = delay + diff;
        } else if (diff >= syncThreshold) {
            delay = 2 * delay;
        }
    }
    return delay;
}
