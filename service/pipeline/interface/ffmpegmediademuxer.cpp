#include "ffmpegmediademuxer.h"

#include "imediasource.h"
#include "../mediasourceinfo.h"

#include <algorithm>
#include <cstdio>

extern "C" {
#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
}

namespace {
constexpr int kDefaultAvioBufferSize = 32 * 1024;
}

FFmpegMediaDemuxer::FFmpegMediaDemuxer()
    : m_source()
    , m_packetSink(nullptr)
    , m_workThread()
    , m_running(false)
    , m_formatContext(nullptr)
    , m_avioContext(nullptr)
    , m_avioBuffer(nullptr)
    , m_audioOutputSampleRate(48000)
    , m_audioOutputChannels(2)
    , m_audioOutputSampleFormat(-1)
    , m_hasVideoInitData(false)
    , m_videoInitData() {}

FFmpegMediaDemuxer::~FFmpegMediaDemuxer() {
    close();
}

void FFmpegMediaDemuxer::setPacketSink(IDemuxPacketSink* sink) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_packetSink = sink;
}

bool FFmpegMediaDemuxer::open(const std::shared_ptr<IMediaSource>& source,
                              MediaSourceInfo* sourceInfo,
                              std::string* errorMessage) {
    close();

    if (!source) {
        setError(errorMessage, "media source is null");
        return false;
    }
    if (!source->isOpen()) {
        setError(errorMessage, "media source is not opened");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_source = source;
    }

    m_avioBuffer = static_cast<std::uint8_t*>(av_malloc(kDefaultAvioBufferSize));
    if (m_avioBuffer == nullptr) {
        setError(errorMessage, "av_malloc failed for avio buffer");
        close();
        return false;
    }

    m_avioContext = avio_alloc_context(
        m_avioBuffer,
        kDefaultAvioBufferSize,
        0,
        this,
        &FFmpegMediaDemuxer::readPacketCallback,
        nullptr,
        source->isSeekable() ? &FFmpegMediaDemuxer::seekCallback : nullptr);
    if (m_avioContext == nullptr) {
        setError(errorMessage, "avio_alloc_context failed");
        close();
        return false;
    }
    m_avioBuffer = nullptr;

    m_formatContext = avformat_alloc_context();
    if (m_formatContext == nullptr) {
        setError(errorMessage, "avformat_alloc_context failed");
        close();
        return false;
    }
    m_formatContext->pb = m_avioContext;
    m_formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

    int ret = avformat_open_input(&m_formatContext, nullptr, nullptr, nullptr);
    if (ret < 0) {
        setError(errorMessage, "avformat_open_input failed");
        close();
        return false;
    }

    ret = avformat_find_stream_info(m_formatContext, nullptr);
    if (ret < 0) {
        setError(errorMessage, "avformat_find_stream_info failed");
        close();
        return false;
    }

    MediaSourceInfo localSourceInfo;
    localSourceInfo.containerFormat =
        (m_formatContext->iformat != nullptr && m_formatContext->iformat->name != nullptr)
            ? m_formatContext->iformat->name
            : "";
    localSourceInfo.durationMs = (m_formatContext->duration > 0)
        ? (m_formatContext->duration / (AV_TIME_BASE / 1000))
        : 0;

    m_hasVideoInitData = false;
    m_videoInitData = DecoderInitData{};

    int videoStreamIndex = -1;
    for (unsigned i = 0; i < m_formatContext->nb_streams; ++i) {
        AVStream* stream = m_formatContext->streams[i];
        if (stream == nullptr || stream->codecpar == nullptr) {
            continue;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = static_cast<int>(i);
            break;
        }
    }

    if (videoStreamIndex >= 0) {
        AVStream* videoStream = m_formatContext->streams[videoStreamIndex];
        localSourceInfo.videoCodec = avcodec_get_name(videoStream->codecpar->codec_id);
        localSourceInfo.width = videoStream->codecpar->width;
        localSourceInfo.height = videoStream->codecpar->height;
        if (videoStream->avg_frame_rate.num > 0 && videoStream->avg_frame_rate.den > 0) {
            localSourceInfo.frameRate = av_q2d(videoStream->avg_frame_rate);
        }

        m_hasVideoInitData = true;
        m_videoInitData.trackType = TrackType::Video;
        m_videoInitData.streamIndex = videoStreamIndex;
        m_videoInitData.codecId = CodecId::FFmpegNative;
        m_videoInitData.nativeCodecId = static_cast<int>(videoStream->codecpar->codec_id);
        m_videoInitData.timeBaseNum = videoStream->time_base.num;
        m_videoInitData.timeBaseDen = videoStream->time_base.den;
        m_videoInitData.startTimeMs = videoStream->start_time != AV_NOPTS_VALUE
            ? av_rescale_q(videoStream->start_time, videoStream->time_base, AVRational{1, 1000})
            : 0;
        m_videoInitData.durationMs = videoStream->duration > 0
            ? av_rescale_q(videoStream->duration, videoStream->time_base, AVRational{1, 1000})
            : localSourceInfo.durationMs;
        m_videoInitData.width = videoStream->codecpar->width;
        m_videoInitData.height = videoStream->codecpar->height;
        m_videoInitData.pixelFormat = videoStream->codecpar->format;
        m_videoInitData.sampleAspectRatioNum = videoStream->sample_aspect_ratio.num > 0
            ? videoStream->sample_aspect_ratio.num
            : 1;
        m_videoInitData.sampleAspectRatioDen = videoStream->sample_aspect_ratio.den > 0
            ? videoStream->sample_aspect_ratio.den
            : 1;
        if (videoStream->codecpar->extradata != nullptr && videoStream->codecpar->extradata_size > 0) {
            const uint8_t* begin = videoStream->codecpar->extradata;
            const uint8_t* end = begin + videoStream->codecpar->extradata_size;
            m_videoInitData.extraData.assign(begin, end);
        }
        if (localSourceInfo.frameRate > 0.0) {
            m_videoInitData.defaultFrameDurationMs = static_cast<int64_t>(1000.0 / localSourceInfo.frameRate);
        }
    }

    if (sourceInfo != nullptr) {
        *sourceInfo = localSourceInfo;
    }

    m_running.store(true, std::memory_order_release);
    m_workThread = std::thread(&FFmpegMediaDemuxer::runLoop, this);
    setError(errorMessage, "");
    return true;
}

void FFmpegMediaDemuxer::close() {
    m_running.store(false, std::memory_order_release);

    if (m_workThread.joinable()) {
        if (m_workThread.get_id() != std::this_thread::get_id()) {
            m_workThread.join();
        } else {
            m_workThread.detach();
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    releaseContexts();
    m_source.reset();
    m_hasVideoInitData = false;
    m_videoInitData = DecoderInitData{};
}

bool FFmpegMediaDemuxer::isOpen() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_formatContext != nullptr;
}

bool FFmpegMediaDemuxer::getVideoDecoderInitData(DecoderInitData* initData) const {
    if (initData == nullptr) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_hasVideoInitData) {
        return false;
    }
    *initData = m_videoInitData;
    return true;
}

void FFmpegMediaDemuxer::seek(int64_t positionMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_formatContext == nullptr) {
        return;
    }

    const int64_t clampedPositionMs = std::max<int64_t>(0, positionMs);
    const int64_t target = av_rescale_q(clampedPositionMs, AVRational{1, 1000}, AV_TIME_BASE_Q);
    const int seekResult = av_seek_frame(m_formatContext, -1, target, AVSEEK_FLAG_BACKWARD);
    IDemuxPacketSink* packetSink = m_packetSink;
    if (seekResult >= 0) {
        avformat_flush(m_formatContext);
    } else if (packetSink != nullptr) {
        packetSink->onDemuxError("demux seek failed");
    }
}

void FFmpegMediaDemuxer::setAudioOutputFormat(int sampleRate, int channels, int sampleFormat) {
    if (sampleRate > 0) {
        m_audioOutputSampleRate = sampleRate;
    }
    if (channels > 0) {
        m_audioOutputChannels = channels;
    }
    if (sampleFormat >= 0) {
        m_audioOutputSampleFormat = sampleFormat;
    }
}

int FFmpegMediaDemuxer::readPacketCallback(void* opaque, std::uint8_t* buffer, int bufferSize) {
    if (opaque == nullptr) {
        return AVERROR(EINVAL);
    }
    auto* demuxer = static_cast<FFmpegMediaDemuxer*>(opaque);
    return demuxer->readPacket(buffer, bufferSize);
}

int64_t FFmpegMediaDemuxer::seekCallback(void* opaque, int64_t offset, int whence) {
    if (opaque == nullptr) {
        return AVERROR(EINVAL);
    }
    auto* demuxer = static_cast<FFmpegMediaDemuxer*>(opaque);
    return demuxer->seekInput(offset, whence);
}

int FFmpegMediaDemuxer::readPacket(std::uint8_t* buffer, int bufferSize) {
    std::shared_ptr<IMediaSource> source;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        source = m_source;
    }

    if (!source) {
        return AVERROR_EOF;
    }
    std::string errorMessage;
    const int readSize = source->read(buffer, bufferSize, &errorMessage);
    if (readSize < 0) {
        return AVERROR(EIO);
    }
    if (readSize == 0) {
        return AVERROR_EOF;
    }
    return readSize;
}

int64_t FFmpegMediaDemuxer::seekInput(int64_t offset, int whence) {
    std::shared_ptr<IMediaSource> source;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        source = m_source;
    }
    if (!source) {
        return AVERROR(EIO);
    }

    if (whence == AVSEEK_SIZE) {
        return source->sizeBytes();
    }

    MediaSourceSeekOrigin origin = MediaSourceSeekOrigin::Begin;
    if ((whence & ~AVSEEK_FORCE) == SEEK_SET) {
        origin = MediaSourceSeekOrigin::Begin;
    } else if ((whence & ~AVSEEK_FORCE) == SEEK_CUR) {
        origin = MediaSourceSeekOrigin::Current;
    } else if ((whence & ~AVSEEK_FORCE) == SEEK_END) {
        origin = MediaSourceSeekOrigin::End;
    } else {
        return AVERROR(EINVAL);
    }

    std::string errorMessage;
    if (!source->seek(offset, origin, &errorMessage)) {
        return AVERROR(EIO);
    }
    return source->tell();
}

void FFmpegMediaDemuxer::runLoop() {
    while (m_running.load(std::memory_order_acquire)) {
        AVPacket* avPacket = av_packet_alloc();
        if (avPacket == nullptr) {
            IDemuxPacketSink* packetSink = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                packetSink = m_packetSink;
            }
            if (packetSink != nullptr) {
                packetSink->onDemuxError("av_packet_alloc failed");
            }
            return;
        }

        int readResult = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_formatContext == nullptr) {
                av_packet_free(&avPacket);
                return;
            }
            readResult = av_read_frame(m_formatContext, avPacket);
        }

        if (readResult < 0) {
            av_packet_free(&avPacket);
            IDemuxPacketSink* packetSink = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                packetSink = m_packetSink;
            }
            if (readResult == AVERROR_EOF) {
                if (packetSink != nullptr) {
                    packetSink->onDemuxEndOfStream();
                }
            } else if (packetSink != nullptr) {
                packetSink->onDemuxError("av_read_frame failed");
            }
            return;
        }

        AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;
        AVRational timeBase{1, 1};
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_formatContext != nullptr &&
                avPacket->stream_index >= 0 &&
                avPacket->stream_index < static_cast<int>(m_formatContext->nb_streams)) {
                const AVStream* stream = m_formatContext->streams[avPacket->stream_index];
                if (stream != nullptr && stream->codecpar != nullptr) {
                    mediaType = stream->codecpar->codec_type;
                    timeBase = stream->time_base;
                }
            }
        }

        TrackType trackType = TrackType::Video;
        if (mediaType == AVMEDIA_TYPE_VIDEO) {
            trackType = TrackType::Video;
        } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
            trackType = TrackType::Audio;
        } else if (mediaType == AVMEDIA_TYPE_SUBTITLE) {
            trackType = TrackType::Subtitle;
        } else {
            av_packet_free(&avPacket);
            continue;
        }

        auto packet = std::make_shared<EncodedPacket>();
        packet->track = trackType;
        packet->streamIndex = avPacket->stream_index;
        packet->codecId = CodecId::FFmpegNative;
        packet->data = avPacket->data;
        packet->size = avPacket->size;
        packet->posBytes = avPacket->pos;

        if ((avPacket->flags & AV_PKT_FLAG_KEY) != 0) {
            packet->flags |= PacketFlagKey;
        }
        if ((avPacket->flags & AV_PKT_FLAG_CORRUPT) != 0) {
            packet->flags |= PacketFlagCorrupt;
        }
        if ((avPacket->flags & AV_PKT_FLAG_DISCARD) != 0) {
            packet->flags |= PacketFlagDiscard;
        }

        const int64_t timestamp = (avPacket->pts != AV_NOPTS_VALUE) ? avPacket->pts : avPacket->dts;
        if (timestamp != AV_NOPTS_VALUE) {
            packet->ptsMs = av_rescale_q(timestamp, timeBase, AVRational{1, 1000});
        }
        if (avPacket->dts != AV_NOPTS_VALUE) {
            packet->dtsMs = av_rescale_q(avPacket->dts, timeBase, AVRational{1, 1000});
        }
        if (avPacket->duration > 0) {
            packet->durationMs = av_rescale_q(avPacket->duration, timeBase, AVRational{1, 1000});
        }

        auto holder = std::shared_ptr<void>(avPacket, [](void* p) {
            AVPacket* packetPtr = static_cast<AVPacket*>(p);
            av_packet_free(&packetPtr);
        });
        packet->payloadOwner = holder;
        packet->nativeHandle = holder;

        IDemuxPacketSink* packetSink = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            packetSink = m_packetSink;
        }
        if (packetSink == nullptr || !packetSink->onDemuxPacket(packet)) {
            return;
        }
    }
}

void FFmpegMediaDemuxer::releaseContexts() {
    if (m_formatContext != nullptr) {
        avformat_close_input(&m_formatContext);
    }
    if (m_avioContext != nullptr) {
        avio_context_free(&m_avioContext);
    }
    if (m_avioBuffer != nullptr) {
        av_free(m_avioBuffer);
        m_avioBuffer = nullptr;
    }
}

void FFmpegMediaDemuxer::setError(std::string* errorMessage, const std::string& message) {
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}
