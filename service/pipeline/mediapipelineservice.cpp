#include "mediapipelineservice.h"

#include "factory/mediabackendfactory.h"
#include "interface/blockingframequeue.h"
#include "interface/blockingpacketqueue.h"
#include "interface/decoderinitdata.h"
#include "interface/ipipelineevents.h"
#include "interface/imediasource.h"
#include "interface/ivideodecoder.h"

MediaPipelineService::MediaPipelineService()
    : m_backendFactory(std::make_unique<MediaBackendFactory>())
    , m_mediaSource()
    , m_demuxer()
    , m_videoDecoder()
    , m_videoPacketQueue(std::make_unique<BlockingPacketQueue>(128))
    , m_audioPacketQueue(std::make_unique<BlockingPacketQueue>(128))
    , m_subtitlePacketQueue(std::make_unique<BlockingPacketQueue>(64))
    , m_videoFrameQueue(std::make_unique<BlockingFrameQueue>(16))
    , m_audioFrameQueue(std::make_unique<BlockingFrameQueue>(32))
    , m_events(nullptr)
    , m_audioOutputSampleRate(48000)
    , m_audioOutputChannels(2)
    , m_audioOutputSampleFormat(-1) {}

MediaPipelineService::~MediaPipelineService() {
    closeMedia();
}

void MediaPipelineService::setEvents(IPipelineEvents* events) {
    m_events = events;
}

bool MediaPipelineService::openMedia(const std::string& filePath, MediaSourceInfo* sourceInfo, std::string* errorMessage) {
    if (filePath.empty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "file path is empty";
        }
        if (m_events != nullptr) {
            m_events->onOpenMediaFailed(filePath, "file path is empty");
        }
        return false;
    }

    closeMedia();
    m_videoPacketQueue->init(128);
    m_audioPacketQueue->init(128);
    m_subtitlePacketQueue->init(64);
    m_videoFrameQueue->init(16);
    m_audioFrameQueue->init(32);

    MediaSourceFactoryConfig sourceConfig;
    sourceConfig.backendType = MediaSourceBackendType::LocalFile;
    m_mediaSource = m_backendFactory->createMediaSource(sourceConfig);
    if (!m_mediaSource) {
        if (errorMessage != nullptr) {
            *errorMessage = "create media source failed";
        }
        if (m_events != nullptr) {
            m_events->onOpenMediaFailed(filePath, "create media source failed");
        }
        return false;
    }

    std::string sourceError;
    if (!m_mediaSource->open(filePath, &sourceError)) {
        if (errorMessage != nullptr) {
            *errorMessage = sourceError;
        }
        if (m_events != nullptr) {
            m_events->onOpenMediaFailed(filePath, sourceError);
        }
        return false;
    }

    DemuxerComponentConfig demuxerConfig;
    demuxerConfig.backendType = DemuxerBackendType::FFmpeg;
    m_demuxer = m_backendFactory->createDemuxer(demuxerConfig);
    if (!m_demuxer) {
        if (errorMessage != nullptr) {
            *errorMessage = "create demuxer failed";
        }
        if (m_events != nullptr) {
            m_events->onOpenMediaFailed(filePath, "create demuxer failed");
        }
        return false;
    }
    m_demuxer->setPacketSink(this);
    m_demuxer->setAudioOutputFormat(m_audioOutputSampleRate, m_audioOutputChannels, m_audioOutputSampleFormat);

    MediaSourceInfo localSourceInfo;
    std::string demuxError;
    if (!m_demuxer->open(m_mediaSource, &localSourceInfo, &demuxError)) {
        if (errorMessage != nullptr) {
            *errorMessage = demuxError;
        }
        if (m_events != nullptr) {
            m_events->onOpenMediaFailed(filePath, demuxError);
        }
        return false;
    }
    localSourceInfo.filePath = filePath;

    VideoDecoderComponentConfig videoDecoderConfig;
    videoDecoderConfig.backendType = VideoDecoderBackendType::FFmpeg;
    m_videoDecoder = m_backendFactory->createVideoDecoder(videoDecoderConfig);
    if (!m_videoDecoder) {
        if (errorMessage != nullptr) {
            *errorMessage = "create video decoder failed";
        }
        if (m_events != nullptr) {
            m_events->onOpenMediaFailed(filePath, "create video decoder failed");
        }
        return false;
    }

    DecoderInitData initData;
    if (!m_demuxer->getVideoDecoderInitData(&initData)) {
        if (errorMessage != nullptr) {
            *errorMessage = "demuxer does not provide video decoder init data";
        }
        if (m_events != nullptr) {
            m_events->onOpenMediaFailed(filePath, "demuxer does not provide video decoder init data");
        }
        return false;
    }

    std::string decoderError;
    m_videoDecoder->bindQueues(m_videoPacketQueue.get(), m_videoFrameQueue.get());
    if (!m_videoDecoder->configure(initData, &decoderError)) {
        if (errorMessage != nullptr) {
            *errorMessage = decoderError;
        }
        if (m_events != nullptr) {
            m_events->onOpenMediaFailed(filePath, decoderError);
        }
        return false;
    }
    m_videoDecoder->start();

    if (sourceInfo != nullptr) {
        *sourceInfo = localSourceInfo;
    }
    if (m_events != nullptr) {
        m_events->onOpenMediaSucceeded(localSourceInfo);
    }
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}

void MediaPipelineService::closeMedia() {
    if (m_videoDecoder) {
        m_videoDecoder->stop();
        m_videoDecoder.reset();
    }
    if (m_demuxer) {
        m_demuxer->close();
        m_demuxer.reset();
    }
    if (m_mediaSource) {
        m_mediaSource->close();
        m_mediaSource.reset();
    }

    if (m_videoPacketQueue) {
        m_videoPacketQueue->abort();
        m_videoPacketQueue->clear();
    }
    if (m_audioPacketQueue) {
        m_audioPacketQueue->abort();
        m_audioPacketQueue->clear();
    }
    if (m_subtitlePacketQueue) {
        m_subtitlePacketQueue->abort();
        m_subtitlePacketQueue->clear();
    }
    if (m_videoFrameQueue) {
        m_videoFrameQueue->abort();
        m_videoFrameQueue->clear();
    }
    if (m_audioFrameQueue) {
        m_audioFrameQueue->abort();
        m_audioFrameQueue->clear();
    }
}

void MediaPipelineService::seek(int64_t positionMs) {
    if (m_videoPacketQueue) {
        m_videoPacketQueue->flushForSeek();
    }
    if (m_audioPacketQueue) {
        m_audioPacketQueue->flushForSeek();
    }
    if (m_subtitlePacketQueue) {
        m_subtitlePacketQueue->flushForSeek();
    }
    if (m_videoFrameQueue) {
        m_videoFrameQueue->flushForSeek();
    }
    if (m_audioFrameQueue) {
        m_audioFrameQueue->flushForSeek();
    }

    if (m_videoDecoder) {
        m_videoDecoder->flushForSeek();
    }
    if (m_demuxer) {
        m_demuxer->seek(positionMs);
    }
    if (m_events != nullptr) {
        m_events->onSeekCompleted(positionMs);
    }
}

void MediaPipelineService::setAudioOutputFormat(int sampleRate, int channels, int sampleFormat) {
    if (sampleRate > 0) {
        m_audioOutputSampleRate = sampleRate;
    }
    if (channels > 0) {
        m_audioOutputChannels = channels;
    }
    if (sampleFormat >= 0) {
        m_audioOutputSampleFormat = sampleFormat;
    }
    if (m_demuxer) {
        m_demuxer->setAudioOutputFormat(m_audioOutputSampleRate, m_audioOutputChannels, m_audioOutputSampleFormat);
    }
}

std::shared_ptr<Frame> MediaPipelineService::peekNextVideoFrame() {
    return m_videoFrameQueue ? m_videoFrameQueue->peek() : nullptr;
}

std::shared_ptr<Frame> MediaPipelineService::popVideoFrame() {
    return m_videoFrameQueue ? m_videoFrameQueue->pop(0) : nullptr;
}

std::shared_ptr<Frame> MediaPipelineService::peekNextAudioFrame() {
    return m_audioFrameQueue ? m_audioFrameQueue->peek() : nullptr;
}

std::shared_ptr<Frame> MediaPipelineService::popAudioFrame() {
    return m_audioFrameQueue ? m_audioFrameQueue->pop(0) : nullptr;
}

bool MediaPipelineService::onDemuxPacket(const EncodedPacketPtr& packet) {
    if (!packet) {
        return false;
    }

    PacketQueuePushStatus status = PacketQueuePushStatus::Ok;
    switch (packet->track) {
    case TrackType::Video:
        status = m_videoPacketQueue->push(packet);
        break;
    case TrackType::Audio:
        // New pipeline audio decode path is not connected yet.
        // Drop audio packets to avoid packet queue backpressure blocking demux.
        return true;
    case TrackType::Subtitle:
        // Subtitle decode path is not connected yet.
        // Drop subtitle packets to avoid packet queue backpressure blocking demux.
        return true;
    default:
        return true;
    }
    return status == PacketQueuePushStatus::Ok;
}

void MediaPipelineService::onDemuxEndOfStream() {
    if (m_videoPacketQueue) {
        m_videoPacketQueue->close();
    }
    if (m_audioPacketQueue) {
        m_audioPacketQueue->close();
    }
    if (m_subtitlePacketQueue) {
        m_subtitlePacketQueue->close();
    }
}

void MediaPipelineService::onDemuxError(const std::string& errorMessage) {
    if (m_events != nullptr) {
        m_events->onPipelineError(errorMessage);
    }
}
