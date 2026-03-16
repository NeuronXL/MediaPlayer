#ifndef FFMPEGVIDEODECODER_H
#define FFMPEGVIDEODECODER_H

#include "workerconfig.h"
#include "framequeue.h"
#include "packetqueue.h"

#include <cstdint>
#include <thread>

struct AVCodecContext;
struct AVFrame;

class FFmpegVideoDecoder {
    public:
    explicit FFmpegVideoDecoder(PacketQueue* videoPacketQueue, FrameQueue* videoFrameQueue);
    ~FFmpegVideoDecoder();

    public:
    void configure(const VideoDecoderConfig& config);

    private:
    void runLoop();
    void shutDwon();

    private:
    std::thread m_workThread;
    PacketQueue* m_videoPacketQueue;
    FrameQueue* m_videoFrameQueue;
    AVCodecContext* m_codecContext;
    AVFrame* m_frame;
    int m_timeBaseNum;
    int m_timeBaseDen;
    int64_t m_frameIntervalUs;
};

#endif // FFMPEGVIDEODECODER_H
