#include "mediaplayerengine.h"

#include "../logging/logservice.h"
#include "../pipeline/mediapipelineservice.h"

#include <chrono>
#include <string>

MediaPlayerEngine::MediaPlayerEngine(std::shared_ptr<IVideoAdapter> videoAdapter,
                                     std::shared_ptr<IAudioAdapter> audioAdapter)
    : m_pipelineService(new MediaPipelineService())
    , m_videoAdapter(std::move(videoAdapter))
    , m_audioAdapter(std::move(audioAdapter))
    , m_masterClockType(MasterClockType::Audio)
    , m_playState(PlayState::Paused)
    , m_curPts(0){
    m_videoFeedThread = std::thread(&MediaPlayerEngine::videoFeed, this);
    m_audioFeedThread = std::thread(&MediaPlayerEngine::audioFeed, this);
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
    m_pipelineService->openMedia(filePath);
    m_videoClock.setCurrentTime(0);
    m_audioClock.setCurrentTime(0);
}

void MediaPlayerEngine::videoFeed() {
    int64_t delay = 10;
    while (true) {
        if (m_playState == PlayState::Paused) {
            delay = 10;
        } else if (m_playState == PlayState::Playing) {
            FramePtr nextFrame = m_pipelineService->peekNextVideoFrame();
            if (!nextFrame)
                goto nextVideoLoop;

            m_videoClock.setCurrentTime(nextFrame->pts);
            if (auto videoFrame = std::dynamic_pointer_cast<VideoFrame>(nextFrame)) {
                if (m_videoAdapter) {
                    m_videoAdapter->onVideoFrame(videoFrame);
                }
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
    int64_t diff, syncThreshold = 0;
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
