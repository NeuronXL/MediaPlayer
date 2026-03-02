#ifndef FFMPEGFRAMECONVERTER_H
#define FFMPEGFRAMECONVERTER_H

#include <QImage>

struct AVFrame;

class FFmpegFrameConverter
{
  public:
    FFmpegFrameConverter();
    ~FFmpegFrameConverter();

    QImage toQImage(const AVFrame* frame) const;
};

#endif // FFMPEGFRAMECONVERTER_H
