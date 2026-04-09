#ifndef FFMPEGMEDIADEMUXER_H
#define FFMPEGMEDIADEMUXER_H

#include "imediademuxer.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class IMediaSource;
struct AVFormatContext;
struct AVIOContext;

class FFmpegMediaDemuxer : public IMediaDemuxer {
public:
    FFmpegMediaDemuxer();
    ~FFmpegMediaDemuxer() override;

    void setPacketSink(IDemuxPacketSink* sink) override;
    bool open(const std::shared_ptr<IMediaSource>& source,
              MediaSourceInfo* sourceInfo,
              std::string* errorMessage) override;
    void close() override;
    bool isOpen() const override;
    bool getVideoDecoderInitData(DecoderInitData* initData) const override;

    void seek(int64_t positionMs) override;
    void setAudioOutputFormat(int sampleRate, int channels, int sampleFormat) override;

private:
    static int readPacketCallback(void* opaque, std::uint8_t* buffer, int bufferSize);
    static int64_t seekCallback(void* opaque, int64_t offset, int whence);

    int readPacket(std::uint8_t* buffer, int bufferSize);
    int64_t seekInput(int64_t offset, int whence);
    void runLoop();
    void releaseContexts();
    static void setError(std::string* errorMessage, const std::string& message);

private:
    mutable std::mutex m_mutex;
    std::shared_ptr<IMediaSource> m_source;
    IDemuxPacketSink* m_packetSink;
    std::thread m_workThread;
    std::atomic_bool m_running;

    AVFormatContext* m_formatContext;
    AVIOContext* m_avioContext;
    std::uint8_t* m_avioBuffer;

    int m_audioOutputSampleRate;
    int m_audioOutputChannels;
    int m_audioOutputSampleFormat;
    bool m_hasVideoInitData;
    DecoderInitData m_videoInitData;
};

#endif // FFMPEGMEDIADEMUXER_H
