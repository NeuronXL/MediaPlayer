#include "ffmpegaudiodecoder.h"

#include <memory>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_par.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mathematics.h>
}

FFmpegAudioDecoder::FFmpegAudioDecoder(PacketQueue* audioPacketQueue, FrameQueue* audioFrameQueue)
    : m_audioPacketQueue(audioPacketQueue), m_audioFrameQueue(audioFrameQueue),
      m_codecContext(nullptr), m_frame(nullptr), m_timeBaseNum(1), m_timeBaseDen(1000),
      m_frameIntervalMs(20) {}

FFmpegAudioDecoder::~FFmpegAudioDecoder() {
    shutDwon();
}

void FFmpegAudioDecoder::configure(const AudioDecoderConfig& config) {
    shutDwon();

    if (config.codecParameters == nullptr) {
        throw std::runtime_error("FFmpegAudioDecoder::configure failed: codecParameters is null.");
    }

    const AVCodec* codec = avcodec_find_decoder(config.codecParameters->codec_id);
    if (codec == nullptr) {
        throw std::runtime_error(
            "FFmpegAudioDecoder::configure failed: avcodec_find_decoder returned null.");
    }

    m_codecContext = avcodec_alloc_context3(codec);
    if (m_codecContext == nullptr) {
        throw std::runtime_error(
            "FFmpegAudioDecoder::configure failed: avcodec_alloc_context3 returned null.");
    }

    int ret = avcodec_parameters_to_context(m_codecContext, config.codecParameters);
    if (ret < 0) {
        shutDwon();
        throw std::runtime_error(
            "FFmpegAudioDecoder::configure failed: avcodec_parameters_to_context.");
    }

    ret = avcodec_open2(m_codecContext, codec, nullptr);
    if (ret < 0) {
        shutDwon();
        throw std::runtime_error("FFmpegAudioDecoder::configure failed: avcodec_open2.");
    }

    m_frame = av_frame_alloc();
    if (m_frame == nullptr) {
        shutDwon();
        throw std::runtime_error(
            "FFmpegAudioDecoder::configure failed: av_frame_alloc returned null.");
    }

    m_timeBaseNum = config.timeBaseNum > 0 ? config.timeBaseNum : 1;
    m_timeBaseDen = config.timeBaseDen > 0 ? config.timeBaseDen : 1000;
    m_frameIntervalMs = config.frameIntervalMs > 0 ? config.frameIntervalMs : 20;

    m_workThread = std::thread(&FFmpegAudioDecoder::runLoop, this);
}

void FFmpegAudioDecoder::runLoop() {
    if (m_audioPacketQueue == nullptr || m_audioFrameQueue == nullptr ||
        m_codecContext == nullptr || m_frame == nullptr) {
        return;
    }

    while (true) {
        Packet* packetWrapper = m_audioPacketQueue->pop();
        if (packetWrapper == nullptr) {
            break;
        }

        AVPacket* packet = packetWrapper->packet;
        const int sendResult = avcodec_send_packet(m_codecContext, packet);
        if (packet != nullptr) {
            av_packet_free(&packet);
        }
        packetWrapper->packet = nullptr;
        delete packetWrapper;

        if (sendResult < 0 && sendResult != AVERROR_EOF) {
            continue;
        }

        while (true) {
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

            auto decodedFrame = std::make_shared<AudioFrame>();
            decodedFrame->frame = av_frame_clone(m_frame);
            if (decodedFrame->frame == nullptr) {
                av_frame_unref(m_frame);
                continue;
            }

            decodedFrame->sampleRate = m_frame->sample_rate;
            decodedFrame->channels = m_frame->ch_layout.nb_channels;
            decodedFrame->format = m_frame->format;
            decodedFrame->nbSamples = m_frame->nb_samples;

            const AVRational sourceTimeBase{m_timeBaseNum, m_timeBaseDen};
            const AVRational msTimeBase{1, 1000};
            decodedFrame->pts = m_frame->best_effort_timestamp != AV_NOPTS_VALUE
                ? av_rescale_q(m_frame->best_effort_timestamp, sourceTimeBase, msTimeBase)
                : -1;
            decodedFrame->duration = m_frame->duration > 0
                ? av_rescale_q(m_frame->duration, sourceTimeBase, msTimeBase)
                : m_frameIntervalMs;
            decodedFrame->pos = m_frame->pkt_dts;

            if (!m_audioFrameQueue->push(decodedFrame)) {
                av_frame_unref(m_frame);
                return;
            }

            av_frame_unref(m_frame);
        }
    }
}

void FFmpegAudioDecoder::shutDwon() {
    if (m_frame != nullptr) {
        av_frame_free(&m_frame);
    }
    if (m_codecContext != nullptr) {
        avcodec_free_context(&m_codecContext);
    }
}
