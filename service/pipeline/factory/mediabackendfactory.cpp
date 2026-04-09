#include "mediabackendfactory.h"

#include "ffmpegcomponentfactory.h"
#include "filemediasourcefactory.h"

MediaBackendFactory::MediaBackendFactory()
    : m_mediaSourceFactory(std::make_unique<FileMediaSourceFactory>())
    , m_componentFactory(std::make_unique<FFmpegComponentFactory>()) {}

MediaBackendFactory::MediaBackendFactory(std::unique_ptr<IMediaSourceFactory> mediaSourceFactory,
                                         std::unique_ptr<IBackendComponentFactory> componentFactory)
    : m_mediaSourceFactory(std::move(mediaSourceFactory))
    , m_componentFactory(std::move(componentFactory)) {}

std::shared_ptr<IMediaSource> MediaBackendFactory::createMediaSource(const MediaSourceFactoryConfig& config) const {
    if (!m_mediaSourceFactory) {
        return nullptr;
    }
    return m_mediaSourceFactory->createSource(config);
}

std::shared_ptr<IMediaDemuxer> MediaBackendFactory::createDemuxer(const DemuxerComponentConfig& config) const {
    if (!m_componentFactory) {
        return nullptr;
    }
    return m_componentFactory->createDemuxer(config);
}

std::shared_ptr<IVideoDecoder> MediaBackendFactory::createVideoDecoder(const VideoDecoderComponentConfig& config) const {
    if (!m_componentFactory) {
        return nullptr;
    }
    return m_componentFactory->createVideoDecoder(config);
}
