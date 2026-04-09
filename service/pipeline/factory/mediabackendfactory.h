#ifndef MEDIABACKENDFACTORY_H
#define MEDIABACKENDFACTORY_H

#include "ibackendfactory.h"
#include "ibackendcomponentfactory.h"
#include "imediasourcefactory.h"

#include <memory>

class IMediaDemuxer;
class IMediaSource;
class IVideoDecoder;

class MediaBackendFactory : public IBackendFactory {
public:
    MediaBackendFactory();
    MediaBackendFactory(std::unique_ptr<IMediaSourceFactory> mediaSourceFactory,
                        std::unique_ptr<IBackendComponentFactory> componentFactory);

    std::shared_ptr<IMediaSource> createMediaSource(const MediaSourceFactoryConfig& config) const override;
    std::shared_ptr<IMediaDemuxer> createDemuxer(const DemuxerComponentConfig& config) const override;
    std::shared_ptr<IVideoDecoder> createVideoDecoder(const VideoDecoderComponentConfig& config) const override;

private:
    std::unique_ptr<IMediaSourceFactory> m_mediaSourceFactory;
    std::unique_ptr<IBackendComponentFactory> m_componentFactory;
};

#endif // MEDIABACKENDFACTORY_H
