#ifndef FFMPEGAUDIOFRAMETRANSFORMER_H
#define FFMPEGAUDIOFRAMETRANSFORMER_H

#include "iframetransformer.h"

struct SwrContext;

class FFmpegAudioFrameTransformer : public IAudioFrameTransformer {
public:
    FFmpegAudioFrameTransformer();
    ~FFmpegAudioFrameTransformer() override;

    void configure(const AudioFrameTransformConfig& config) override;
    std::shared_ptr<AudioFrame> transform(const std::shared_ptr<AudioFrame>& srcFrame,
                                          std::string* errorMessage) override;
    void reset() override;

private:
    bool ensureResampler(const AudioFrame& srcFrame, std::string* errorMessage);
    static void setError(std::string* errorMessage, const std::string& message);

private:
    AudioFrameTransformConfig m_config;
    SwrContext* m_swrContext = nullptr;

    int m_cachedSrcSampleRate = 0;
    int m_cachedSrcChannels = 0;
    int m_cachedSrcFormat = -1;
    int m_cachedDstSampleRate = 0;
    int m_cachedDstChannels = 0;
    int m_cachedDstFormat = -1;
};

#endif // FFMPEGAUDIOFRAMETRANSFORMER_H
