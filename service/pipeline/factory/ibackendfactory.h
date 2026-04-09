#ifndef IBACKENDFACTORY_H
#define IBACKENDFACTORY_H

#include "ibackendcomponentfactory.h"
#include "imediasourcefactory.h"

#include <memory>

class IMediaDemuxer;
class IMediaSource;
class IVideoDecoder;

class IBackendFactory {
public:
    virtual ~IBackendFactory() = default;

    virtual std::shared_ptr<IMediaSource> createMediaSource(const MediaSourceFactoryConfig& config) const = 0;
    virtual std::shared_ptr<IMediaDemuxer> createDemuxer(const DemuxerComponentConfig& config) const = 0;
    virtual std::shared_ptr<IVideoDecoder> createVideoDecoder(const VideoDecoderComponentConfig& config) const = 0;
};

#endif // IBACKENDFACTORY_H
