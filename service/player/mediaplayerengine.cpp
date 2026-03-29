#include "mediaplayerengine.h"

#include "../logging/logservice.h"
#include "../pipeline/mediapipelineservice.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <utility>

MediaPlayerEngine::MediaPlayerEngine(std::shared_ptr<IVideoAdapter> videoAdapter,
                                     std::shared_ptr<IAudioAdapter> audioAdapter)
    : m_pipelineService(new MediaPipelineService())
    , m_audioAdapter(std::move(audioAdapter))
    , m_activeAudioFrame()
    , m_activeAudioOffset(0)
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
    if (m_audioAdapter) {
        m_audioAdapter->setAudioFrameSource(this);
        const AudioOutputSpec spec = m_audioAdapter->outputSpec();
        m_pipelineService->setAudioOutputFormat(spec.sampleRate, spec.channels, spec.sampleFormat);
    }
    m_videoFeedThread = std::thread(&MediaPlayerEngine::videoFeed, this);
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
}

void MediaPlayerEngine::play() {
    m_playState = PlayState::Playing;
    if (m_audioAdapter) {
        m_audioAdapter->start();
    }
}
void MediaPlayerEngine::pause() {
    m_playState = PlayState::Paused;
    if (m_audioAdapter) {
        m_audioAdapter->pause();
    }
}

void MediaPlayerEngine::openMedia(std::string filePath) {
    m_filePath = filePath;
    MediaSourceInfo sourceInfo;
    std::string errorMessage;
    const bool opened = m_pipelineService->openMedia(filePath, &sourceInfo, &errorMessage);
    m_videoClock.setCurrentTime(0);
    m_audioClock.setCurrentTime(0);
    m_activeAudioFrame.reset();
    m_activeAudioOffset = 0;

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

int MediaPlayerEngine::readPcm(std::uint8_t* destination, int maxBytes) {
    if (destination == nullptr || maxBytes <= 0) {
        return 0;
    }
    if (m_playState != PlayState::Playing) {
        std::memset(destination, 0, static_cast<size_t>(maxBytes));
        return maxBytes;
    }

    int writtenBytes = 0;
    while (writtenBytes < maxBytes) {
        if (!m_activeAudioFrame || m_activeAudioOffset >= m_activeAudioFrame->pcmData.size()) {
            m_activeAudioFrame.reset();
            m_activeAudioOffset = 0;

            FramePtr nextFrame = m_pipelineService->peekNextAudioFrame();
            if (!nextFrame) {
                break;
            }

            auto audioFrame = std::dynamic_pointer_cast<AudioFrame>(nextFrame);
            auto consumedFrame = m_pipelineService->popAudioFrame();
            (void)consumedFrame;
            if (!audioFrame || audioFrame->pcmData.empty()) {
                continue;
            }

            m_activeAudioFrame = std::move(audioFrame);
            m_activeAudioOffset = 0;
        }

        const int remainRequest = maxBytes - writtenBytes;
        const std::size_t frameRemainBytes = m_activeAudioFrame->pcmData.size() - m_activeAudioOffset;
        const std::size_t copyBytes = std::min<std::size_t>(frameRemainBytes, static_cast<std::size_t>(remainRequest));
        if (copyBytes == 0) {
            break;
        }

        std::memcpy(destination + writtenBytes, m_activeAudioFrame->pcmData.data() + m_activeAudioOffset, copyBytes);
        writtenBytes += static_cast<int>(copyBytes);
        m_activeAudioOffset += copyBytes;

        if (m_activeAudioFrame->sampleRate > 0 &&
            m_activeAudioFrame->channels > 0 &&
            m_activeAudioFrame->bytesPerSample > 0 &&
            m_activeAudioFrame->pts >= 0) {
            const int64_t bytesPerSecond = static_cast<int64_t>(m_activeAudioFrame->sampleRate) *
                                           static_cast<int64_t>(m_activeAudioFrame->channels) *
                                           static_cast<int64_t>(m_activeAudioFrame->bytesPerSample);
            if (bytesPerSecond > 0) {
                const int64_t playedInFrameMs =
                    static_cast<int64_t>((m_activeAudioOffset * 1000ULL) / static_cast<unsigned long long>(bytesPerSecond));
                m_audioClock.setCurrentTime(m_activeAudioFrame->pts + playedInFrameMs);
            }
        }
    }
    if (writtenBytes < maxBytes) {
        std::memset(destination + writtenBytes, 0, static_cast<size_t>(maxBytes - writtenBytes));
        return maxBytes;
    }
    return writtenBytes;
}
