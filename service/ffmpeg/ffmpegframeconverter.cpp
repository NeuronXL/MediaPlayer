#include "ffmpegframeconverter.h"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

FFmpegFrameConverter::FFmpegFrameConverter() = default;

FFmpegFrameConverter::~FFmpegFrameConverter() = default;

QImage FFmpegFrameConverter::toQImage(const AVFrame* frame) const
{
    if (frame == nullptr || frame->width <= 0 || frame->height <= 0) {
        return {};
    }

    QImage image(frame->width, frame->height, QImage::Format_RGB888);
    if (image.isNull()) {
        return {};
    }

    SwsContext* swsContext = sws_getContext(frame->width, frame->height,
                                            static_cast<AVPixelFormat>(frame->format),
                                            frame->width, frame->height,
                                            AV_PIX_FMT_RGB24, SWS_BILINEAR,
                                            nullptr, nullptr, nullptr);
    if (swsContext == nullptr) {
        return {};
    }

    uint8_t* destinationData[4] = {image.bits(), nullptr, nullptr, nullptr};
    int destinationLineSize[4] = {static_cast<int>(image.bytesPerLine()), 0, 0, 0};

    const int convertedHeight =
        sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
                  destinationData, destinationLineSize);

    sws_freeContext(swsContext);

    if (convertedHeight <= 0) {
        return {};
    }

    return image;
}
