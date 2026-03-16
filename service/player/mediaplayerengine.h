#ifndef MEDIAPLAYERENGINE_H
#define MEDIAPLAYERENGINE_H

#include "../adapter/ivideoadapter.h"
#include "../adapter/iaudioadapter.h"
#include "mediaclock.h"
#include "playstate.h"

#include <cstdint>
#include <memory>
#include <string>
#include <thread>

class MediaPipelineService;

class MediaPlayerEngine {
public:
    explicit MediaPlayerEngine(std::shared_ptr<IVideoAdapter> videoAdapter,
                               std::shared_ptr<IAudioAdapter> audioAdapter);
    ~MediaPlayerEngine();

    void play();
    void pause();
    void openMedia(std::string filePath);

    void videoFeed();
    void audioFeed();

private:
    std::thread m_videoFeedThread;
    std::thread m_audioFeedThread;
    MediaPipelineService* m_pipelineService;
    std::shared_ptr<IVideoAdapter> m_videoAdapter;
    std::shared_ptr<IAudioAdapter> m_audioAdapter;
    MediaClock m_clock;
    PlayState m_playState;
    std::string m_filePath;
    int64_t m_curPts;
};

#endif // MEDIAPLAYERENGINE_H
