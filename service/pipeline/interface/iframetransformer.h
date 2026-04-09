#ifndef IFRAMETRANSFORMER_H
#define IFRAMETRANSFORMER_H

#include "../../common/frame.h"

#include <memory>
#include <string>

struct VideoFrameTransformConfig {
    int dstWidth = 0;
    int dstHeight = 0;
    int dstPixelFormat = -1;
    int scaleFlags = 0;
};

struct AudioFrameTransformConfig {
    int dstSampleRate = 0;
    int dstChannels = 0;
    int dstSampleFormat = -1;
};

class IVideoFrameTransformer {
public:
    virtual ~IVideoFrameTransformer() = default;

    virtual void configure(const VideoFrameTransformConfig& config) = 0;
    virtual std::shared_ptr<VideoFrame> transform(const std::shared_ptr<VideoFrame>& srcFrame,
                                                  std::string* errorMessage) = 0;
    virtual void reset() = 0;
};

class IAudioFrameTransformer {
public:
    virtual ~IAudioFrameTransformer() = default;

    virtual void configure(const AudioFrameTransformConfig& config) = 0;
    virtual std::shared_ptr<AudioFrame> transform(const std::shared_ptr<AudioFrame>& srcFrame,
                                                  std::string* errorMessage) = 0;
    virtual void reset() = 0;
};

#endif // IFRAMETRANSFORMER_H
