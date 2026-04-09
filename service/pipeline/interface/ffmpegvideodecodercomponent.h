#ifndef FFMPEGVIDEODECODERCOMPONENT_H
#define FFMPEGVIDEODECODERCOMPONENT_H

#include "ivideodecoder.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

struct AVCodecContext;
struct AVFrame;

class FFmpegVideoDecoderComponent : public IVideoDecoder {
public:
    FFmpegVideoDecoderComponent();
    ~FFmpegVideoDecoderComponent() override;

    bool configure(const DecoderInitData& initData, std::string* errorMessage) override;
    void bindQueues(IPacketQueue* packetQueue, IFrameQueue* frameQueue) override;

    void start() override;
    void stop() override;
    void flushForSeek() override;

    bool isRunning() const override;

private:
    void runLoop();
    void releaseDecoder();
    bool openDecoder(const DecoderInitData& initData, std::string* errorMessage);
    static void setError(std::string* errorMessage, const std::string& message);

private:
    IPacketQueue* m_packetQueue = nullptr;
    IFrameQueue* m_frameQueue = nullptr;

    AVCodecContext* m_codecContext = nullptr;
    AVFrame* m_frame = nullptr;
    int m_timeBaseNum = 1;
    int m_timeBaseDen = 1000;
    int64_t m_defaultFrameDurationMs = 33;
    int m_packetSerial = -1;

    std::thread m_workThread;
    std::atomic_bool m_running = false;
    std::atomic_bool m_stopRequested = false;
    std::atomic_bool m_flushRequested = false;
    mutable std::mutex m_mutex;
};

#endif // FFMPEGVIDEODECODERCOMPONENT_H
