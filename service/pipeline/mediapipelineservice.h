#ifndef MEDIAPIPELINESERVICE_H
#define MEDIAPIPELINESERVICE_H

#include "factory/ibackendfactory.h"
#include "interface/imediademuxer.h"
#include "interface/imediapipeline.h"
#include "interface/ipacketqueue.h"
#include "interface/iframequeue.h"

#include <cstdint>
#include <memory>
#include <string>

class IPipelineEvents;
class IMediaSource;
class IMediaDemuxer;
class IVideoDecoder;
class IBackendFactory;

class MediaPipelineService : public IMediaPipeline, public IDemuxPacketSink {
public:
    explicit MediaPipelineService();
    ~MediaPipelineService() override;

    void setEvents(IPipelineEvents* events) override;
    bool openMedia(const std::string& filePath, MediaSourceInfo* sourceInfo, std::string* errorMessage) override;
    void closeMedia() override;
    void seek(int64_t positionMs) override;
    void setAudioOutputFormat(int sampleRate, int channels, int sampleFormat) override;

    std::shared_ptr<Frame> peekNextVideoFrame() override;
    std::shared_ptr<Frame> popVideoFrame() override;
    std::shared_ptr<Frame> peekNextAudioFrame() override;
    std::shared_ptr<Frame> popAudioFrame() override;

    bool onDemuxPacket(const EncodedPacketPtr& packet) override;
    void onDemuxEndOfStream() override;
    void onDemuxError(const std::string& errorMessage) override;

private:
    std::unique_ptr<IBackendFactory> m_backendFactory;
    std::shared_ptr<IMediaSource> m_mediaSource;
    std::shared_ptr<IMediaDemuxer> m_demuxer;
    std::shared_ptr<IVideoDecoder> m_videoDecoder;

    std::unique_ptr<IPacketQueue> m_videoPacketQueue;
    std::unique_ptr<IPacketQueue> m_audioPacketQueue;
    std::unique_ptr<IPacketQueue> m_subtitlePacketQueue;
    std::unique_ptr<IFrameQueue> m_videoFrameQueue;
    std::unique_ptr<IFrameQueue> m_audioFrameQueue;

    IPipelineEvents* m_events;
    int m_audioOutputSampleRate;
    int m_audioOutputChannels;
    int m_audioOutputSampleFormat;
};

#endif // MEDIAPIPELINESERVICE_H
