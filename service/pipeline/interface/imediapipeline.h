#ifndef IMEDIAPIPELINE_H
#define IMEDIAPIPELINE_H

#include "../../common/frame.h"
#include "../mediasourceinfo.h"

#include <cstdint>
#include <memory>
#include <string>

class IPipelineEvents;

class IMediaPipeline {
public:
    virtual ~IMediaPipeline() = default;

    virtual void setEvents(IPipelineEvents* events) = 0;

    virtual bool openMedia(const std::string& filePath, MediaSourceInfo* sourceInfo, std::string* errorMessage) = 0;
    virtual void closeMedia() = 0;
    virtual void seek(int64_t positionMs) = 0;
    virtual void setAudioOutputFormat(int sampleRate, int channels, int sampleFormat) = 0;

    virtual std::shared_ptr<Frame> peekNextVideoFrame() = 0;
    virtual std::shared_ptr<Frame> popVideoFrame() = 0;
    virtual std::shared_ptr<Frame> peekNextAudioFrame() = 0;
    virtual std::shared_ptr<Frame> popAudioFrame() = 0;
};

#endif // IMEDIAPIPELINE_H
