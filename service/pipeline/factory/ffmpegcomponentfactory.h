#ifndef FFMPEGCOMPONENTFACTORY_H
#define FFMPEGCOMPONENTFACTORY_H

#include "ibackendcomponentfactory.h"

class FFmpegComponentFactory : public IBackendComponentFactory {
public:
    std::shared_ptr<IMediaDemuxer> createDemuxer(const DemuxerComponentConfig& config) const override;
    std::shared_ptr<IVideoDecoder> createVideoDecoder(const VideoDecoderComponentConfig& config) const override;
};

#endif // FFMPEGCOMPONENTFACTORY_H
