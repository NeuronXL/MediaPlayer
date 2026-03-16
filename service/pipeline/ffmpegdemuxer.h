#ifndef FFMPEGDEMUXER_H
#define FFMPEGDEMUXER_H

#include "packetqueue.h"
#include "ffmpegaudiodecoder.h"
#include "ffmpegvideodecoder.h"

#include <cstdint>
#include <string>
#include <vector>
#include <thread>

struct AVFormatContext;
class FrameQueue;

class FFmpegDemuxer {
    public:
    explicit FFmpegDemuxer(PacketQueue* videoPacketQueue, PacketQueue* audioPacketQueue,
                           PacketQueue* subtitlePacketQueue, FrameQueue* videoFrameQueue,
                           FrameQueue* audioFrameQueue);
    ~FFmpegDemuxer();

    void open(const std::string& filePath);
    void seek(int64_t positionMs);

    private:
    void runLoop();
    void shutDown();

    void resetDemuxSession();

    private:
    std::thread m_workThread;
    FFmpegVideoDecoder m_videoDecoder;
    FFmpegAudioDecoder m_audioDecoder;

    PacketQueue* m_videoPacketQueue;
    PacketQueue* m_audioPacketQueue;
    PacketQueue* m_subtitlePacketQueue;
    FrameQueue* m_videoFrameQueue;
    FrameQueue* m_audioFrameQueue;
    AVFormatContext* m_formatContext;
    std::string m_filePath;
    std::vector<int> m_videoStreams;
    std::vector<int> m_audioStreams;
    std::vector<int> m_subtitleStreams;
    int m_videoStreamIndex;
    int m_audioStreamIndex;
    int m_subtitleStreamIndex;
};

#endif // FFMPEGDEMUXER_H
