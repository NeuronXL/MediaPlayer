#ifndef MEDIAPIPELINESERVICE_H
#define MEDIAPIPELINESERVICE_H

#include <cstdint>
#include <string>

#include "framequeue.h"
#include "mediasourceinfo.h"
#include "packetqueue.h"

class FFmpegDemuxer;
class FFmpegVideoDecoder;

class MediaPipelineService {
    public:
    explicit MediaPipelineService();
    ~MediaPipelineService();

    void closeMedia();
    bool openMedia(std::string& filePath, MediaSourceInfo* sourceInfo, std::string* errorMessage);
    void seek(int64_t positionMs);

    FramePtr peekLastVideoFrame();
    FramePtr peekNextVideoFrame();
    FramePtr popVideoFrame();
    FramePtr peekNextAudioFrame();
    FramePtr popAudioFrame();

    private:
    FFmpegDemuxer* m_demuxer;
    FFmpegVideoDecoder* m_videoDecoder;
    PacketQueue m_videoPacketQueue;
    PacketQueue m_audioPacketQueue;
    PacketQueue m_subtitlePacketQueue;
    FrameQueue m_videoFrameQueue;
    FrameQueue m_audioFrameQueue;
};

#endif // MEDIAPIPELINESERVICE_H
