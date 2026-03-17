#ifndef VIDEORENDERERADAPTER_H
#define VIDEORENDERERADAPTER_H

#include <QImage>
#include <QVariant>

#include <memory>
#include <unordered_map>

struct AVFrame;

enum class VideoRenderTarget
{
    QImage
};

class IVideoFrameConverter
{
  public:
    virtual ~IVideoFrameConverter() = default;
    virtual QVariant convert(const AVFrame* frame) const = 0;
};

class VideoRendererAdapter
{
  public:
    VideoRendererAdapter();
    ~VideoRendererAdapter();

    QVariant adapt(const AVFrame* frame, VideoRenderTarget target) const;
    QImage adaptToQImage(const AVFrame* frame) const;
    bool hasTarget(VideoRenderTarget target) const;
    void registerConverter(VideoRenderTarget target,
                           std::unique_ptr<IVideoFrameConverter> converter);

  private:
    std::unordered_map<int, std::unique_ptr<IVideoFrameConverter>> m_converters;
};

#endif // VIDEORENDERERADAPTER_H
