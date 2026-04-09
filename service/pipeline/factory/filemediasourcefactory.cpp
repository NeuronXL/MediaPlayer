#include "filemediasourcefactory.h"

#include "../interface/filemediasource.h"

std::shared_ptr<IMediaSource> FileMediaSourceFactory::createSource(const MediaSourceFactoryConfig& config) const {
    switch (config.backendType) {
    case MediaSourceBackendType::LocalFile:
        return std::make_shared<FileMediaSource>();
    default:
        return nullptr;
    }
}
