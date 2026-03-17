#include "videorendereradapter.h"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

class QImageVideoFrameConverter final : public IVideoFrameConverter
{
  public:
    QVariant convert(const AVFrame* frame) const override
    {
        if (frame == nullptr || frame->width <= 0 || frame->height <= 0) {
            return {};
        }

        QImage image(frame->width, frame->height, QImage::Format_RGB888);
        if (image.isNull()) {
            return {};
        }

        SwsContext* swsContext = sws_getContext(
            frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
            frame->width, frame->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr,
            nullptr, nullptr);
        if (swsContext == nullptr) {
            return {};
        }

        uint8_t* destinationData[4] = {image.bits(), nullptr, nullptr, nullptr};
        int destinationLineSize[4] = {static_cast<int>(image.bytesPerLine()), 0, 0,
                                      0};

        const int convertedHeight = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, destinationData, destinationLineSize);
        sws_freeContext(swsContext);

        if (convertedHeight <= 0) {
            return {};
        }

        return image;
    }
};

VideoRendererAdapter::VideoRendererAdapter()
{
    registerConverter(VideoRenderTarget::QImage,
                      std::make_unique<QImageVideoFrameConverter>());
}

VideoRendererAdapter::~VideoRendererAdapter() = default;

void VideoRendererAdapter::registerConverter(
    VideoRenderTarget target, std::unique_ptr<IVideoFrameConverter> converter)
{
    if (converter == nullptr) {
        return;
    }

    m_converters[static_cast<int>(target)] = std::move(converter);
}

bool VideoRendererAdapter::hasTarget(VideoRenderTarget target) const
{
    return m_converters.find(static_cast<int>(target)) != m_converters.end();
}

QVariant VideoRendererAdapter::adapt(const AVFrame* frame,
                                     VideoRenderTarget target) const
{
    const auto it = m_converters.find(static_cast<int>(target));
    if (it == m_converters.end() || it->second == nullptr) {
        return {};
    }

    return it->second->convert(frame);
}

QImage VideoRendererAdapter::adaptToQImage(const AVFrame* frame) const
{
    const QVariant data = adapt(frame, VideoRenderTarget::QImage);
    if (!data.canConvert<QImage>()) {
        return {};
    }

    return data.value<QImage>();
}
