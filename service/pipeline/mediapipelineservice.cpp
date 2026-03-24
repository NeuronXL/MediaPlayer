#include "mediapipelineservice.h"

#include "ffmpegdemuxer.h"

MediaPipelineService::MediaPipelineService()
    : m_demuxer(new FFmpegDemuxer(&m_videoPacketQueue, &m_audioPacketQueue, &m_subtitlePacketQueue,
                                  &m_videoFrameQueue, &m_audioFrameQueue)),
      m_videoDecoder(nullptr) {
    m_videoPacketQueue.init();
    m_audioPacketQueue.init();
    m_subtitlePacketQueue.init();
    m_audioFrameQueue.init();
}

MediaPipelineService::~MediaPipelineService() {
    closeMedia();

    delete m_demuxer;
    m_demuxer = nullptr;

    delete m_videoDecoder;
    m_videoDecoder = nullptr;
}

bool MediaPipelineService::openMedia(std::string& filePath, MediaSourceInfo* sourceInfo, std::string* errorMessage) {
    if (filePath.empty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "file path is empty";
        }
        return false;
    }

    closeMedia();
    m_videoPacketQueue.init();
    m_audioPacketQueue.init();
    m_subtitlePacketQueue.init();
    m_videoFrameQueue.init();
    m_audioFrameQueue.init();

    if (m_demuxer != nullptr) {
        return m_demuxer->open(filePath, sourceInfo, errorMessage);
    }
    if (errorMessage != nullptr) {
        *errorMessage = "demuxer is null";
    }
    return false;
}

void MediaPipelineService::closeMedia() {
    m_videoPacketQueue.abort();
    m_audioPacketQueue.abort();
    m_subtitlePacketQueue.abort();
    m_videoFrameQueue.abort();
    m_audioFrameQueue.abort();
}

void MediaPipelineService::seek(int64_t positionMs) {
    if (m_demuxer != nullptr) {
        m_demuxer->seek(positionMs);
    }
}
FramePtr MediaPipelineService::peekLastVideoFrame() {
    return m_videoFrameQueue.peekLast();
}
FramePtr MediaPipelineService::peekNextVideoFrame() {
    return m_videoFrameQueue.peekNext();
}
FramePtr MediaPipelineService::popVideoFrame() {
    return m_videoFrameQueue.pop();
}
FramePtr MediaPipelineService::peekNextAudioFrame() {
    return m_audioFrameQueue.peekNext();
}
FramePtr MediaPipelineService::popAudioFrame() {
    return m_audioFrameQueue.pop();
}
