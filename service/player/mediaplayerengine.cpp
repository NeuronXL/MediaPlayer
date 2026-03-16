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
    , m_playState(PlayState::Paused)
    , m_curPts(0) {
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
    m_audioAdapter->start();
}
void MediaPlayerEngine::pause() {
    m_curPts = m_clock.getCurrentTime();
}

void MediaPlayerEngine::openMedia(std::string filePath) {
    m_pipelineService->openMedia(filePath);
    m_clock.setCurrentTime(0);
}

void MediaPlayerEngine::videoFeed() {
    while (true) {
        int64_t target = 10000;
        if (m_playState == PlayState::Paused) {
            continue;
        } else if (m_playState == PlayState::Playing) {
            FramePtr nextFrame = m_pipelineService->peekNextVideoFrame();
            int64_t currentTime = m_clock.getCurrentTime();
            if (nextFrame) {
                if (nextFrame->pts <= currentTime) {
                    if (nextFrame->pts + nextFrame->duration <= currentTime) {
                        auto droppedFrame = m_pipelineService->popVideoFrame();
                        (void)droppedFrame;
                        continue;
                    } else {
                        // LogService::instance().append(
                        //     "dispatch frame pts=" + std::to_string(nextFrame->pts) +
                        //     " duration=" + std::to_string(nextFrame->duration) +
                        //     " currentTime=" + std::to_string(currentTime));
                        if (auto videoFrame = std::dynamic_pointer_cast<VideoFrame>(nextFrame)) {
                            if (m_videoAdapter) {
                                m_videoAdapter->onVideoFrame(videoFrame);
                            }
                        }
                        target = (nextFrame->pts + nextFrame->duration - currentTime) > 0
                            ? nextFrame->pts + nextFrame->duration - currentTime
                            : 1000;
                        auto displayedFrame = m_pipelineService->popVideoFrame();
                        (void)displayedFrame;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(target));
    }
}
void MediaPlayerEngine::audioFeed() {
    while (true) {
        int64_t target = 300000;
        if (m_playState == PlayState::Paused) {
            if (m_audioAdapter) {
                m_audioAdapter->pause();
            }
            m_curPts = m_clock.getCurrentTime();
        } else if (m_playState == PlayState::Playing) {
            if (m_audioAdapter) {
                m_audioAdapter->start();
            }
            int64_t time = m_audioAdapter->playedTimeUs();
            m_clock.setCurrentTime(m_audioAdapter->playedTimeUs());
            FramePtr nextFrame = m_pipelineService->peekNextAudioFrame();
            if (nextFrame) {
                if (auto audioFrame = std::dynamic_pointer_cast<AudioFrame>(nextFrame)) {
                    if (m_audioAdapter) {
                        m_audioAdapter->write(audioFrame);
                    }
                }
                auto consumedFrame = m_pipelineService->popAudioFrame();
                (void)consumedFrame;
            } else {
                target = 10000;
            }
            LogService::instance().append(
                            "playedTimeUs=" + std::to_string(time) +
                            "target=" + std::to_string(target));
        }
        std::this_thread::sleep_for(std::chrono::microseconds(target));
    }
}
