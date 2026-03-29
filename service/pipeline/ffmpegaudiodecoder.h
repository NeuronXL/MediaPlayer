#ifndef FFMPEGAUDIODECODER_H
#define FFMPEGAUDIODECODER_H

#include "framequeue.h"
#include "packetqueue.h"
#include "workerconfig.h"

#include <cstdint>
#include <thread>

struct AVCodecContext;
struct AVFrame;

class FFmpegAudioDecoder {
    public:
    explicit FFmpegAudioDecoder(PacketQueue* audioPacketQueue, FrameQueue* audioFrameQueue);
    ~FFmpegAudioDecoder();

    public:
    void configure(const AudioDecoderConfig& config);

    private:
    void runLoop();
    void shutDwon();

    private:
    std::thread m_workThread;
    PacketQueue* m_audioPacketQueue;
    FrameQueue* m_audioFrameQueue;
    AVCodecContext* m_codecContext;
    AVFrame* m_frame;
    int m_timeBaseNum;
    int m_timeBaseDen;
    int64_t m_frameIntervalMs;
    int m_outputSampleRate;
    int m_outputChannels;
    int m_outputSampleFormat;
};

#endif // FFMPEGAUDIODECODER_H
