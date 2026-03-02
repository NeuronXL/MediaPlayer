#include "ffmpegframeconverter.h"

struct AVFrame;

FFmpegFrameConverter::FFmpegFrameConverter() = default;

FFmpegFrameConverter::~FFmpegFrameConverter() = default;

QImage FFmpegFrameConverter::toQImage(const AVFrame* frame) const
{
    Q_UNUSED(frame);
    // TODO: Convert decoded AVFrame data to a displayable QImage.
    return {};
}
