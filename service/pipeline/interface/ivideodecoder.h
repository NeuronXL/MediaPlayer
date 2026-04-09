#ifndef IVIDEODECODER_H
#define IVIDEODECODER_H

#include "decoderinitdata.h"

#include <string>

class IPacketQueue;
class IFrameQueue;

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    virtual bool configure(const DecoderInitData& initData, std::string* errorMessage) = 0;
    virtual void bindQueues(IPacketQueue* packetQueue, IFrameQueue* frameQueue) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void flushForSeek() = 0;

    virtual bool isRunning() const = 0;
};

#endif // IVIDEODECODER_H
