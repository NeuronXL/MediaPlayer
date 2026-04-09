#ifndef FILEMEDIASOURCEFACTORY_H
#define FILEMEDIASOURCEFACTORY_H

#include "imediasourcefactory.h"

class FileMediaSourceFactory : public IMediaSourceFactory {
public:
    std::shared_ptr<IMediaSource> createSource(const MediaSourceFactoryConfig& config) const override;
};

#endif // FILEMEDIASOURCEFACTORY_H
