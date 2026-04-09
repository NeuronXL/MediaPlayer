#include "ffmpegcomponentfactory.h"

#include "../interface/ffmpegmediademuxer.h"
#include "../interface/ffmpegvideodecodercomponent.h"

std::shared_ptr<IMediaDemuxer> FFmpegComponentFactory::createDemuxer(const DemuxerComponentConfig& config) const {
    switch (config.backendType) {
    case DemuxerBackendType::FFmpeg:
        return std::make_shared<FFmpegMediaDemuxer>();
    default:
        return nullptr;
    }
}

std::shared_ptr<IVideoDecoder> FFmpegComponentFactory::createVideoDecoder(const VideoDecoderComponentConfig& config) const {
    switch (config.backendType) {
    case VideoDecoderBackendType::FFmpeg:
        return std::make_shared<FFmpegVideoDecoderComponent>();
    default:
        return nullptr;
    }
}
