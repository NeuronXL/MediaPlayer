#ifndef IBACKENDCOMPONENTFACTORY_H
#define IBACKENDCOMPONENTFACTORY_H

#include <memory>

class IMediaDemuxer;
class IVideoDecoder;

enum class DemuxerBackendType {
    FFmpeg
};

enum class VideoDecoderBackendType {
    FFmpeg
};

struct DemuxerComponentConfig {
    DemuxerBackendType backendType = DemuxerBackendType::FFmpeg;
};

struct VideoDecoderComponentConfig {
    VideoDecoderBackendType backendType = VideoDecoderBackendType::FFmpeg;
};

class IBackendComponentFactory {
public:
    virtual ~IBackendComponentFactory() = default;

    virtual std::shared_ptr<IMediaDemuxer> createDemuxer(const DemuxerComponentConfig& config) const = 0;
    virtual std::shared_ptr<IVideoDecoder> createVideoDecoder(const VideoDecoderComponentConfig& config) const = 0;
};

#endif // IBACKENDCOMPONENTFACTORY_H
