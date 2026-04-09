#ifndef IPIPELINEEVENTS_H
#define IPIPELINEEVENTS_H

#include "../mediasourceinfo.h"

#include <cstdint>
#include <string>

class IPipelineEvents {
public:
    virtual ~IPipelineEvents() = default;

    virtual void onOpenMediaSucceeded(const MediaSourceInfo& mediaInfo) = 0;
    virtual void onOpenMediaFailed(const std::string& filePath, const std::string& errorMessage) = 0;
    virtual void onPipelineError(const std::string& errorMessage) = 0;
    virtual void onSeekCompleted(int64_t positionMs) = 0;
};

#endif // IPIPELINEEVENTS_H
