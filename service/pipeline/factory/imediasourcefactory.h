#ifndef IMEDIASOURCEFACTORY_H
#define IMEDIASOURCEFACTORY_H

#include <memory>

class IMediaSource;

enum class MediaSourceBackendType {
    LocalFile
};

struct MediaSourceFactoryConfig {
    MediaSourceBackendType backendType = MediaSourceBackendType::LocalFile;
};

class IMediaSourceFactory {
public:
    virtual ~IMediaSourceFactory() = default;
    virtual std::shared_ptr<IMediaSource> createSource(const MediaSourceFactoryConfig& config) const = 0;
};

#endif // IMEDIASOURCEFACTORY_H
