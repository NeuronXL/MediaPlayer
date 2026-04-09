#ifndef MEDIAPLAYERENGINE_H
#define MEDIAPLAYERENGINE_H

#include "../adapter/ivideoadapter.h"
#include "../adapter/iaudioadapter.h"
#include "../pipeline/interface/ipipelineevents.h"
#include "../pipeline/interface/imediapipeline.h"
#include "iaudioframesource.h"
#include "engineevent.h"
#include "iplaybackscheduler.h"
#include "masterclocktype.h"
#include "mediaclock.h"
#include "playstate.h"

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class MediaPlayerEngine : public IAudioFrameSource, public IPipelineEvents {
public:
    using SubscriptionId = std::uint64_t;
    using EventHandler = std::function<void(const EngineEvent&)>;
    using EventExecutor = std::function<void(std::function<void()>)>;

    explicit MediaPlayerEngine(std::shared_ptr<IVideoAdapter> videoAdapter,
                               std::shared_ptr<IAudioAdapter> audioAdapter);
    ~MediaPlayerEngine();

    SubscriptionId subscribe(EngineEventMask eventMask, EventHandler handler, EventExecutor executor = {});
    void unsubscribe(SubscriptionId subscriptionId);

    void play();
    void pause();
    void seek(int64_t positionMs);
    void openMedia(std::string filePath);

    void videoFeed();
    int readPcm(std::uint8_t* destination, int maxBytes) override;

    void onOpenMediaSucceeded(const MediaSourceInfo& mediaInfo) override;
    void onOpenMediaFailed(const std::string& filePath, const std::string& errorMessage) override;
    void onPipelineError(const std::string& errorMessage) override;
    void onSeekCompleted(int64_t positionMs) override;

private:
    int64_t computeClockDelay(int64_t delay);
    void publishEvent(const EngineEvent& event);

private:
    struct Subscriber {
        SubscriptionId id{0};
        EngineEventMask eventMask{0};
        EventHandler handler;
        EventExecutor executor;
    };

    std::thread m_videoFeedThread;
    std::unique_ptr<IMediaPipeline> m_pipelineService;
    std::unique_ptr<IPlaybackScheduler> m_playbackScheduler;
    std::shared_ptr<IVideoAdapter> m_videoAdapter;
    std::shared_ptr<IAudioAdapter> m_audioAdapter;
    std::shared_ptr<AudioFrame> m_activeAudioFrame;
    std::size_t m_activeAudioOffset;
    std::atomic<SubscriptionId> m_nextSubscriptionId;
    std::mutex m_subscriberMutex;
    std::vector<Subscriber> m_subscribers;
    MasterClockType m_masterClockType;
    MediaClock m_videoClock;
    MediaClock m_audioClock;
    PlayState m_playState;
    std::string m_filePath;
    int64_t m_curPts;
};

#endif // MEDIAPLAYERENGINE_H
