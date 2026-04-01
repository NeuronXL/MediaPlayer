#include "ffmpegaudiodecoder.h"

#include <cstring>
#include <memory>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_par.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

FFmpegAudioDecoder::FFmpegAudioDecoder(PacketQueue* audioPacketQueue, FrameQueue* audioFrameQueue)
    : m_audioPacketQueue(audioPacketQueue), m_audioFrameQueue(audioFrameQueue),
      m_codecContext(nullptr), m_frame(nullptr), m_timeBaseNum(1), m_timeBaseDen(1000),
      m_frameIntervalMs(20), m_outputSampleRate(0), m_outputChannels(0),
      m_outputSampleFormat(AV_SAMPLE_FMT_S16), m_packetSerial(-1) {}

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
    m_outputSampleRate = config.outputSampleRate > 0 ? config.outputSampleRate : m_codecContext->sample_rate;
    m_outputChannels = config.outputChannels > 0 ? config.outputChannels : m_codecContext->ch_layout.nb_channels;
    m_outputSampleFormat = config.outputSampleFormat >= 0 ? config.outputSampleFormat : AV_SAMPLE_FMT_S16;

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

        const int packetSerial = packetWrapper->serial;
        if (packetSerial != m_packetSerial) {
            avcodec_flush_buffers(m_codecContext);
            m_packetSerial = packetSerial;
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

            if (packetSerial != m_audioPacketQueue->currentSerial()) {
                avcodec_flush_buffers(m_codecContext);
                av_frame_unref(m_frame);
                break;
            }

            auto decodedFrame = std::make_shared<AudioFrame>();
            const int sourceSampleRate = m_frame->sample_rate > 0
                ? m_frame->sample_rate
                : m_codecContext->sample_rate;
            const int sourceChannels = m_frame->ch_layout.nb_channels > 0
                ? m_frame->ch_layout.nb_channels
                : m_codecContext->ch_layout.nb_channels;
            const AVSampleFormat targetSampleFormat = static_cast<AVSampleFormat>(m_outputSampleFormat);
            decodedFrame->sampleRate = m_outputSampleRate > 0 ? m_outputSampleRate : sourceSampleRate;
            decodedFrame->channels = m_outputChannels > 0 ? m_outputChannels : sourceChannels;
            if (decodedFrame->sampleRate <= 0 || decodedFrame->channels <= 0 || m_frame->nb_samples <= 0) {
                av_frame_unref(m_frame);
                continue;
            }

            decodedFrame->format = m_outputSampleFormat;
            decodedFrame->bytesPerSample = av_get_bytes_per_sample(targetSampleFormat);
            decodedFrame->interleaved = true;
            decodedFrame->nbSamples = m_frame->nb_samples;
            if (decodedFrame->bytesPerSample <= 0) {
                av_frame_unref(m_frame);
                continue;
            }

            const AVRational sourceTimeBase{m_timeBaseNum, m_timeBaseDen};
            const AVRational msTimeBase{1, 1000};
            decodedFrame->pts = m_frame->best_effort_timestamp != AV_NOPTS_VALUE
                ? av_rescale_q(m_frame->best_effort_timestamp, sourceTimeBase, msTimeBase)
                : -1;
            decodedFrame->duration = m_frame->duration > 0
                ? av_rescale_q(m_frame->duration, sourceTimeBase, msTimeBase)
                : m_frameIntervalMs;
            decodedFrame->pos = m_frame->pkt_dts;
            decodedFrame->serial = packetSerial;

            const AVSampleFormat inSampleFmt = static_cast<AVSampleFormat>(m_frame->format);
            const bool canDirectCopy = inSampleFmt == targetSampleFormat &&
                                       av_sample_fmt_is_planar(inSampleFmt) == 0 &&
                                       sourceSampleRate == decodedFrame->sampleRate &&
                                       sourceChannels == decodedFrame->channels;
            if (canDirectCopy) {
                const int directBytes = av_samples_get_buffer_size(
                    nullptr, decodedFrame->channels, decodedFrame->nbSamples, targetSampleFormat, 1);
                if (directBytes > 0 && m_frame->data[0] != nullptr) {
                    decodedFrame->pcmData.resize(static_cast<size_t>(directBytes));
                    std::memcpy(decodedFrame->pcmData.data(), m_frame->data[0], static_cast<size_t>(directBytes));
                } else {
                    av_frame_unref(m_frame);
                    continue;
                }
            } else {
                AVChannelLayout inLayout{};
                AVChannelLayout outLayout{};
                if (m_frame->ch_layout.nb_channels > 0) {
                    if (av_channel_layout_copy(&inLayout, &m_frame->ch_layout) < 0) {
                        av_frame_unref(m_frame);
                        continue;
                    }
                } else {
                    av_channel_layout_default(&inLayout, decodedFrame->channels);
                }
                av_channel_layout_default(&outLayout, decodedFrame->channels);

                SwrContext* swrContext = nullptr;
                const int allocResult = swr_alloc_set_opts2(
                    &swrContext,
                    &outLayout,
                    targetSampleFormat,
                    decodedFrame->sampleRate,
                    &inLayout,
                    inSampleFmt,
                    sourceSampleRate,
                    0,
                    nullptr);
                if (allocResult < 0 || swrContext == nullptr) {
                    av_channel_layout_uninit(&inLayout);
                    av_channel_layout_uninit(&outLayout);
                    swr_free(&swrContext);
                    av_frame_unref(m_frame);
                    continue;
                }

                const int initResult = swr_init(swrContext);
                if (initResult < 0) {
                    av_channel_layout_uninit(&inLayout);
                    av_channel_layout_uninit(&outLayout);
                    swr_free(&swrContext);
                    av_frame_unref(m_frame);
                    continue;
                }

                const int dstSamples = static_cast<int>(av_rescale_rnd(
                    swr_get_delay(swrContext, sourceSampleRate) + m_frame->nb_samples,
                    decodedFrame->sampleRate,
                    sourceSampleRate,
                    AV_ROUND_UP));
                const int dstBytes = av_samples_get_buffer_size(
                    nullptr, decodedFrame->channels, dstSamples, targetSampleFormat, 1);
                if (dstSamples <= 0 || dstBytes <= 0) {
                    av_channel_layout_uninit(&inLayout);
                    av_channel_layout_uninit(&outLayout);
                    swr_free(&swrContext);
                    av_frame_unref(m_frame);
                    continue;
                }

                decodedFrame->pcmData.resize(static_cast<size_t>(dstBytes));
                uint8_t* outData[] = {decodedFrame->pcmData.data()};
                const uint8_t** inData = const_cast<const uint8_t**>(m_frame->extended_data);
                const int convertedSamples = swr_convert(
                    swrContext,
                    outData,
                    dstSamples,
                    inData,
                    m_frame->nb_samples);
                if (convertedSamples <= 0) {
                    av_channel_layout_uninit(&inLayout);
                    av_channel_layout_uninit(&outLayout);
                    swr_free(&swrContext);
                    av_frame_unref(m_frame);
                    continue;
                }

                const int convertedBytes = av_samples_get_buffer_size(
                    nullptr, decodedFrame->channels, convertedSamples, targetSampleFormat, 1);
                if (convertedBytes <= 0) {
                    av_channel_layout_uninit(&inLayout);
                    av_channel_layout_uninit(&outLayout);
                    swr_free(&swrContext);
                    av_frame_unref(m_frame);
                    continue;
                }

                decodedFrame->nbSamples = convertedSamples;
                decodedFrame->pcmData.resize(static_cast<size_t>(convertedBytes));

                av_channel_layout_uninit(&inLayout);
                av_channel_layout_uninit(&outLayout);
                swr_free(&swrContext);
            }

            if (!m_audioFrameQueue->push(decodedFrame)) {
                av_frame_unref(m_frame);
                if (packetSerial != m_audioPacketQueue->currentSerial()) {
                    break;
                }
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
