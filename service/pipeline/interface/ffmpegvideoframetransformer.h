#ifndef FFMPEGVIDEOFRAMETRANSFORMER_H
#define FFMPEGVIDEOFRAMETRANSFORMER_H

#include "iframetransformer.h"

struct SwsContext;

class FFmpegVideoFrameTransformer : public IVideoFrameTransformer {
public:
    FFmpegVideoFrameTransformer();
    ~FFmpegVideoFrameTransformer() override;

    void configure(const VideoFrameTransformConfig& config) override;
    std::shared_ptr<VideoFrame> transform(const std::shared_ptr<VideoFrame>& srcFrame,
                                          std::string* errorMessage) override;
    void reset() override;

private:
    static void setError(std::string* errorMessage, const std::string& message);

private:
    VideoFrameTransformConfig m_config;
    SwsContext* m_swsContext = nullptr;
};

#endif // FFMPEGVIDEOFRAMETRANSFORMER_H
