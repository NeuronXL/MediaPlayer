#include "ffmpegvideoframetransformer.h"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

FFmpegVideoFrameTransformer::FFmpegVideoFrameTransformer() = default;

FFmpegVideoFrameTransformer::~FFmpegVideoFrameTransformer() {
    reset();
}

void FFmpegVideoFrameTransformer::configure(const VideoFrameTransformConfig& config) {
    m_config = config;
}

std::shared_ptr<VideoFrame> FFmpegVideoFrameTransformer::transform(const std::shared_ptr<VideoFrame>& srcFrame,
                                                                   std::string* errorMessage) {
    if (!srcFrame || srcFrame->frame == nullptr || srcFrame->width <= 0 || srcFrame->height <= 0) {
        setError(errorMessage, "invalid video source frame");
        return nullptr;
    }

    const int dstWidth = m_config.dstWidth > 0 ? m_config.dstWidth : srcFrame->width;
    const int dstHeight = m_config.dstHeight > 0 ? m_config.dstHeight : srcFrame->height;
    const AVPixelFormat srcFormat = static_cast<AVPixelFormat>(srcFrame->format);
    const AVPixelFormat dstFormat = m_config.dstPixelFormat >= 0
        ? static_cast<AVPixelFormat>(m_config.dstPixelFormat)
        : srcFormat;
    const int scaleFlags = m_config.scaleFlags != 0 ? m_config.scaleFlags : SWS_BILINEAR;

    m_swsContext = sws_getCachedContext(
        m_swsContext,
        srcFrame->width,
        srcFrame->height,
        srcFormat,
        dstWidth,
        dstHeight,
        dstFormat,
        scaleFlags,
        nullptr,
        nullptr,
        nullptr);
    if (m_swsContext == nullptr) {
        setError(errorMessage, "sws_getCachedContext failed");
        return nullptr;
    }

    AVFrame* dstAvFrame = av_frame_alloc();
    if (dstAvFrame == nullptr) {
        setError(errorMessage, "av_frame_alloc failed");
        return nullptr;
    }
    dstAvFrame->format = dstFormat;
    dstAvFrame->width = dstWidth;
    dstAvFrame->height = dstHeight;
    dstAvFrame->sample_aspect_ratio = srcFrame->sar;

    const int bufferResult = av_frame_get_buffer(dstAvFrame, 32);
    if (bufferResult < 0) {
        av_frame_free(&dstAvFrame);
        setError(errorMessage, "av_frame_get_buffer failed");
        return nullptr;
    }

    const int scaleResult = sws_scale(
        m_swsContext,
        srcFrame->frame->data,
        srcFrame->frame->linesize,
        0,
        srcFrame->height,
        dstAvFrame->data,
        dstAvFrame->linesize);
    if (scaleResult <= 0) {
        av_frame_free(&dstAvFrame);
        setError(errorMessage, "sws_scale failed");
        return nullptr;
    }

    auto transformed = std::make_shared<VideoFrame>();
    transformed->serial = srcFrame->serial;
    transformed->pts = srcFrame->pts;
    transformed->duration = srcFrame->duration;
    transformed->pos = srcFrame->pos;
    transformed->width = dstWidth;
    transformed->height = dstHeight;
    transformed->format = dstFormat;
    transformed->sar = dstAvFrame->sample_aspect_ratio;
    transformed->frame = dstAvFrame;
    setError(errorMessage, "");
    return transformed;
}

void FFmpegVideoFrameTransformer::reset() {
    if (m_swsContext != nullptr) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
}

void FFmpegVideoFrameTransformer::setError(std::string* errorMessage, const std::string& message) {
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}
