#include "ffmpegvideodecodercomponent.h"

#include "iframequeue.h"
#include "ipacketqueue.h"

#include <algorithm>
#include <chrono>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/rational.h>
}

namespace {
constexpr AVRational kMsTimeBase{1, 1000};
}

FFmpegVideoDecoderComponent::FFmpegVideoDecoderComponent() = default;

FFmpegVideoDecoderComponent::~FFmpegVideoDecoderComponent() {
    stop();
    releaseDecoder();
}

bool FFmpegVideoDecoderComponent::configure(const DecoderInitData& initData, std::string* errorMessage) {
    stop();
    releaseDecoder();

    if (initData.trackType != TrackType::Video) {
        setError(errorMessage, "decoder init track type is not video");
        return false;
    }
    if (!openDecoder(initData, errorMessage)) {
        releaseDecoder();
        return false;
    }

    m_timeBaseNum = initData.timeBaseNum > 0 ? initData.timeBaseNum : 1;
    m_timeBaseDen = initData.timeBaseDen > 0 ? initData.timeBaseDen : 1000;
    m_defaultFrameDurationMs = initData.defaultFrameDurationMs > 0 ? initData.defaultFrameDurationMs : 33;
    m_packetSerial = -1;
    setError(errorMessage, "");
    return true;
}

void FFmpegVideoDecoderComponent::bindQueues(IPacketQueue* packetQueue, IFrameQueue* frameQueue) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_packetQueue = packetQueue;
    m_frameQueue = frameQueue;
}

void FFmpegVideoDecoderComponent::start() {
    if (m_running.load(std::memory_order_acquire)) {
        return;
    }
    m_stopRequested.store(false, std::memory_order_release);
    m_flushRequested.store(false, std::memory_order_release);
    m_running.store(true, std::memory_order_release);
    m_workThread = std::thread(&FFmpegVideoDecoderComponent::runLoop, this);
}

void FFmpegVideoDecoderComponent::stop() {
    m_stopRequested.store(true, std::memory_order_release);
    m_running.store(false, std::memory_order_release);
    if (m_workThread.joinable()) {
        if (m_workThread.get_id() != std::this_thread::get_id()) {
            m_workThread.join();
        } else {
            m_workThread.detach();
        }
    }
}

void FFmpegVideoDecoderComponent::flushForSeek() {
    m_flushRequested.store(true, std::memory_order_release);
}

bool FFmpegVideoDecoderComponent::isRunning() const {
    return m_running.load(std::memory_order_acquire);
}

void FFmpegVideoDecoderComponent::runLoop() {
    while (!m_stopRequested.load(std::memory_order_acquire)) {
        IPacketQueue* packetQueue = nullptr;
        IFrameQueue* frameQueue = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            packetQueue = m_packetQueue;
            frameQueue = m_frameQueue;
        }

        if (packetQueue == nullptr || frameQueue == nullptr || m_codecContext == nullptr || m_frame == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if (m_flushRequested.exchange(false, std::memory_order_acq_rel)) {
            avcodec_flush_buffers(m_codecContext);
            m_packetSerial = -1;
        }

        const PacketPopResult popResult = packetQueue->pop(10);
        if (popResult.status == PacketQueuePopStatus::Timeout) {
            continue;
        }
        if (popResult.status == PacketQueuePopStatus::Aborted || popResult.status == PacketQueuePopStatus::Closed) {
            break;
        }
        if (!popResult.packet) {
            continue;
        }

        const EncodedPacketPtr& encoded = popResult.packet;
        const int packetSerial = encoded->serial;
        if (packetSerial != m_packetSerial) {
            avcodec_flush_buffers(m_codecContext);
            m_packetSerial = packetSerial;
        }

        AVPacket* sendPacket = nullptr;
        bool ownedPacket = false;

        if (encoded->nativeHandle) {
            sendPacket = static_cast<AVPacket*>(encoded->nativeHandle.get());
        }

        if (sendPacket == nullptr) {
            sendPacket = av_packet_alloc();
            if (sendPacket == nullptr) {
                continue;
            }
            ownedPacket = true;

            if (encoded->size > 0) {
                const int allocResult = av_new_packet(sendPacket, encoded->size);
                if (allocResult < 0) {
                    av_packet_free(&sendPacket);
                    continue;
                }
                if (encoded->data != nullptr) {
                    std::memcpy(sendPacket->data, encoded->data, static_cast<size_t>(encoded->size));
                }
            }

            sendPacket->stream_index = encoded->streamIndex;
            sendPacket->pos = encoded->posBytes;
            if (encoded->ptsMs >= 0) {
                sendPacket->pts = av_rescale_q(encoded->ptsMs, kMsTimeBase, AVRational{m_timeBaseNum, m_timeBaseDen});
            }
            if (encoded->dtsMs >= 0) {
                sendPacket->dts = av_rescale_q(encoded->dtsMs, kMsTimeBase, AVRational{m_timeBaseNum, m_timeBaseDen});
            }
            if (encoded->durationMs > 0) {
                sendPacket->duration =
                    av_rescale_q(encoded->durationMs, kMsTimeBase, AVRational{m_timeBaseNum, m_timeBaseDen});
            }
            if ((encoded->flags & PacketFlagKey) != 0) {
                sendPacket->flags |= AV_PKT_FLAG_KEY;
            }
            if ((encoded->flags & PacketFlagCorrupt) != 0) {
                sendPacket->flags |= AV_PKT_FLAG_CORRUPT;
            }
            if ((encoded->flags & PacketFlagDiscard) != 0) {
                sendPacket->flags |= AV_PKT_FLAG_DISCARD;
            }
        }

        const int sendResult = avcodec_send_packet(m_codecContext, sendPacket);
        if (ownedPacket) {
            av_packet_free(&sendPacket);
        }
        if (sendResult < 0 && sendResult != AVERROR(EAGAIN) && sendResult != AVERROR_EOF) {
            continue;
        }

        while (!m_stopRequested.load(std::memory_order_acquire)) {
            const int receiveResult = avcodec_receive_frame(m_codecContext, m_frame);
            if (receiveResult == AVERROR(EAGAIN)) {
                break;
            }
            if (receiveResult == AVERROR_EOF) {
                return;
            }
            if (receiveResult < 0) {
                break;
            }

            auto decoded = std::make_shared<VideoFrame>();
            decoded->frame = av_frame_clone(m_frame);
            if (decoded->frame == nullptr) {
                av_frame_unref(m_frame);
                continue;
            }

            decoded->serial = packetSerial;
            decoded->width = m_frame->width;
            decoded->height = m_frame->height;
            decoded->format = m_frame->format;
            decoded->sar = m_frame->sample_aspect_ratio;
            decoded->pos = m_frame->pkt_dts;

            const AVRational sourceTimeBase{m_timeBaseNum, m_timeBaseDen};
            if (m_frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                decoded->pts = av_rescale_q(m_frame->best_effort_timestamp, sourceTimeBase, kMsTimeBase);
            } else {
                decoded->pts = -1;
            }
            if (m_frame->duration > 0) {
                decoded->duration = av_rescale_q(m_frame->duration, sourceTimeBase, kMsTimeBase);
            } else {
                decoded->duration = m_defaultFrameDurationMs;
            }

            if (!frameQueue->push(decoded)) {
                av_frame_unref(m_frame);
                return;
            }
            av_frame_unref(m_frame);
        }
    }
    m_running.store(false, std::memory_order_release);
}

void FFmpegVideoDecoderComponent::releaseDecoder() {
    if (m_frame != nullptr) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_codecContext != nullptr) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }
}

bool FFmpegVideoDecoderComponent::openDecoder(const DecoderInitData& initData, std::string* errorMessage) {
    if (initData.nativeCodecId < 0) {
        setError(errorMessage, "native codec id is invalid");
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(static_cast<AVCodecID>(initData.nativeCodecId));
    if (codec == nullptr) {
        setError(errorMessage, "avcodec_find_decoder failed");
        return false;
    }

    m_codecContext = avcodec_alloc_context3(codec);
    if (m_codecContext == nullptr) {
        setError(errorMessage, "avcodec_alloc_context3 failed");
        return false;
    }

    m_codecContext->time_base = AVRational{
        initData.timeBaseNum > 0 ? initData.timeBaseNum : 1,
        initData.timeBaseDen > 0 ? initData.timeBaseDen : 1000
    };
    m_codecContext->pkt_timebase = m_codecContext->time_base;
    if (initData.width > 0) {
        m_codecContext->width = initData.width;
    }
    if (initData.height > 0) {
        m_codecContext->height = initData.height;
    }
    if (initData.pixelFormat >= 0) {
        m_codecContext->pix_fmt = static_cast<AVPixelFormat>(initData.pixelFormat);
    }
    if (!initData.extraData.empty()) {
        const int allocSize = static_cast<int>(initData.extraData.size()) + AV_INPUT_BUFFER_PADDING_SIZE;
        m_codecContext->extradata = static_cast<uint8_t*>(av_mallocz(static_cast<size_t>(allocSize)));
        if (m_codecContext->extradata == nullptr) {
            setError(errorMessage, "alloc extradata failed");
            return false;
        }
        m_codecContext->extradata_size = static_cast<int>(initData.extraData.size());
        std::memcpy(m_codecContext->extradata, initData.extraData.data(), initData.extraData.size());
    }

    const int openResult = avcodec_open2(m_codecContext, codec, nullptr);
    if (openResult < 0) {
        setError(errorMessage, "avcodec_open2 failed");
        return false;
    }

    m_frame = av_frame_alloc();
    if (m_frame == nullptr) {
        setError(errorMessage, "av_frame_alloc failed");
        return false;
    }
    return true;
}

void FFmpegVideoDecoderComponent::setError(std::string* errorMessage, const std::string& message) {
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}
