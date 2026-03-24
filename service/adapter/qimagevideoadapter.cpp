#include "qimagevideoadapter.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
}

QImageVideoAdapter::QImageVideoAdapter(QObject* parent)
    : QObject(parent) {}

void QImageVideoAdapter::onVideoFrame(const std::shared_ptr<VideoFrame>& frame) {
    if (!frame) {
        return;
    }
    emit frameAdapted(adapt(*frame), frame->pts);
}

QImage QImageVideoAdapter::adapt(const VideoFrame& frame) {
    if (frame.frame == nullptr || frame.width <= 0 || frame.height <= 0) {
        return {};
    }

    QImage image(frame.width, frame.height, QImage::Format_RGB888);
    if (image.isNull()) {
        return {};
    }

    m_swsContext = sws_getCachedContext(
        m_swsContext,
        frame.width,
        frame.height,
        static_cast<AVPixelFormat>(frame.format),
        frame.width,
        frame.height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);
    if (m_swsContext == nullptr) {
        releaseVideoContext();
        return {};
    }
    m_srcWidth = frame.width;
    m_srcHeight = frame.height;
    m_srcFormat = frame.format;
    m_dstWidth = frame.width;
    m_dstHeight = frame.height;
    m_dstFormat = AV_PIX_FMT_RGB24;

    uint8_t* dstData[4] = {image.bits(), nullptr, nullptr, nullptr};
    int dstLinesize[4] = {static_cast<int>(image.bytesPerLine()), 0, 0, 0};
    sws_scale(m_swsContext, frame.frame->data, frame.frame->linesize, 0, frame.height, dstData, dstLinesize);

    return image;
}
