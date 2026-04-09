#include "ffmpegaudioframetransformer.h"

#include <algorithm>
#include <cstring>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

FFmpegAudioFrameTransformer::FFmpegAudioFrameTransformer() = default;

FFmpegAudioFrameTransformer::~FFmpegAudioFrameTransformer() {
    reset();
}

void FFmpegAudioFrameTransformer::configure(const AudioFrameTransformConfig& config) {
    m_config = config;
}

std::shared_ptr<AudioFrame> FFmpegAudioFrameTransformer::transform(const std::shared_ptr<AudioFrame>& srcFrame,
                                                                   std::string* errorMessage) {
    if (!srcFrame || srcFrame->sampleRate <= 0 || srcFrame->channels <= 0 || srcFrame->pcmData.empty()) {
        setError(errorMessage, "invalid audio source frame");
        return nullptr;
    }
    if (!srcFrame->interleaved) {
        setError(errorMessage, "planar pcm is not supported in current transformer");
        return nullptr;
    }

    const int dstSampleRate = m_config.dstSampleRate > 0 ? m_config.dstSampleRate : srcFrame->sampleRate;
    const int dstChannels = m_config.dstChannels > 0 ? m_config.dstChannels : srcFrame->channels;
    const int dstSampleFormat = m_config.dstSampleFormat >= 0 ? m_config.dstSampleFormat : srcFrame->format;

    if (dstSampleRate == srcFrame->sampleRate &&
        dstChannels == srcFrame->channels &&
        dstSampleFormat == srcFrame->format) {
        auto passthrough = std::make_shared<AudioFrame>();
        passthrough->serial = srcFrame->serial;
        passthrough->pts = srcFrame->pts;
        passthrough->duration = srcFrame->duration;
        passthrough->pos = srcFrame->pos;
        passthrough->sampleRate = srcFrame->sampleRate;
        passthrough->channels = srcFrame->channels;
        passthrough->format = srcFrame->format;
        passthrough->nbSamples = srcFrame->nbSamples;
        passthrough->bytesPerSample = srcFrame->bytesPerSample;
        passthrough->interleaved = srcFrame->interleaved;
        passthrough->pcmData = srcFrame->pcmData;
        setError(errorMessage, "");
        return passthrough;
    }

    if (!ensureResampler(*srcFrame, errorMessage)) {
        return nullptr;
    }

    const int srcSampleRate = srcFrame->sampleRate;
    const int srcNbSamples = srcFrame->nbSamples > 0
        ? srcFrame->nbSamples
        : static_cast<int>(srcFrame->pcmData.size()) / std::max(1, srcFrame->bytesPerSample * srcFrame->channels);

    const int maxDstSamples = static_cast<int>(
        av_rescale_rnd(swr_get_delay(m_swrContext, srcSampleRate) + srcNbSamples,
                       dstSampleRate,
                       srcSampleRate,
                       AV_ROUND_UP));
    if (maxDstSamples <= 0) {
        setError(errorMessage, "invalid destination sample count");
        return nullptr;
    }

    uint8_t** dstData = nullptr;
    int dstLinesize = 0;
    const int allocResult = av_samples_alloc_array_and_samples(
        &dstData,
        &dstLinesize,
        dstChannels,
        maxDstSamples,
        static_cast<AVSampleFormat>(dstSampleFormat),
        0);
    if (allocResult < 0 || dstData == nullptr || dstData[0] == nullptr) {
        if (dstData != nullptr) {
            av_freep(&dstData);
        }
        setError(errorMessage, "av_samples_alloc_array_and_samples failed");
        return nullptr;
    }

    const uint8_t* srcData[1] = { srcFrame->pcmData.data() };
    const int convertedSamples = swr_convert(
        m_swrContext,
        dstData,
        maxDstSamples,
        srcData,
        srcNbSamples);
    if (convertedSamples < 0) {
        av_freep(&dstData[0]);
        av_freep(&dstData);
        setError(errorMessage, "swr_convert failed");
        return nullptr;
    }

    const int dstBufferSize = av_samples_get_buffer_size(
        nullptr,
        dstChannels,
        convertedSamples,
        static_cast<AVSampleFormat>(dstSampleFormat),
        1);
    if (dstBufferSize <= 0) {
        av_freep(&dstData[0]);
        av_freep(&dstData);
        setError(errorMessage, "av_samples_get_buffer_size failed");
        return nullptr;
    }

    auto transformed = std::make_shared<AudioFrame>();
    transformed->serial = srcFrame->serial;
    transformed->pts = srcFrame->pts;
    transformed->duration = srcFrame->duration;
    transformed->pos = srcFrame->pos;
    transformed->sampleRate = dstSampleRate;
    transformed->channels = dstChannels;
    transformed->format = dstSampleFormat;
    transformed->nbSamples = convertedSamples;
    transformed->bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(dstSampleFormat));
    transformed->interleaved = true;
    transformed->pcmData.assign(dstData[0], dstData[0] + dstBufferSize);

    av_freep(&dstData[0]);
    av_freep(&dstData);
    setError(errorMessage, "");
    return transformed;
}

void FFmpegAudioFrameTransformer::reset() {
    if (m_swrContext != nullptr) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }
    m_cachedSrcSampleRate = 0;
    m_cachedSrcChannels = 0;
    m_cachedSrcFormat = -1;
    m_cachedDstSampleRate = 0;
    m_cachedDstChannels = 0;
    m_cachedDstFormat = -1;
}

bool FFmpegAudioFrameTransformer::ensureResampler(const AudioFrame& srcFrame, std::string* errorMessage) {
    const int dstSampleRate = m_config.dstSampleRate > 0 ? m_config.dstSampleRate : srcFrame.sampleRate;
    const int dstChannels = m_config.dstChannels > 0 ? m_config.dstChannels : srcFrame.channels;
    const int dstSampleFormat = m_config.dstSampleFormat >= 0 ? m_config.dstSampleFormat : srcFrame.format;

    const bool changed =
        m_swrContext == nullptr ||
        m_cachedSrcSampleRate != srcFrame.sampleRate ||
        m_cachedSrcChannels != srcFrame.channels ||
        m_cachedSrcFormat != srcFrame.format ||
        m_cachedDstSampleRate != dstSampleRate ||
        m_cachedDstChannels != dstChannels ||
        m_cachedDstFormat != dstSampleFormat;
    if (!changed) {
        return true;
    }

    if (m_swrContext != nullptr) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }

    AVChannelLayout srcLayout;
    AVChannelLayout dstLayout;
    av_channel_layout_default(&srcLayout, srcFrame.channels);
    av_channel_layout_default(&dstLayout, dstChannels);

    const int allocResult = swr_alloc_set_opts2(
        &m_swrContext,
        &dstLayout,
        static_cast<AVSampleFormat>(dstSampleFormat),
        dstSampleRate,
        &srcLayout,
        static_cast<AVSampleFormat>(srcFrame.format),
        srcFrame.sampleRate,
        0,
        nullptr);

    av_channel_layout_uninit(&srcLayout);
    av_channel_layout_uninit(&dstLayout);

    if (allocResult < 0 || m_swrContext == nullptr) {
        setError(errorMessage, "swr_alloc_set_opts2 failed");
        return false;
    }

    const int initResult = swr_init(m_swrContext);
    if (initResult < 0) {
        swr_free(&m_swrContext);
        setError(errorMessage, "swr_init failed");
        return false;
    }

    m_cachedSrcSampleRate = srcFrame.sampleRate;
    m_cachedSrcChannels = srcFrame.channels;
    m_cachedSrcFormat = srcFrame.format;
    m_cachedDstSampleRate = dstSampleRate;
    m_cachedDstChannels = dstChannels;
    m_cachedDstFormat = dstSampleFormat;
    return true;
}

void FFmpegAudioFrameTransformer::setError(std::string* errorMessage, const std::string& message) {
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}
