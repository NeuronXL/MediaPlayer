#ifndef IFRAMEQUEUE_H
#define IFRAMEQUEUE_H

#include "../../common/frame.h"

#include <memory>

using DecodedFramePtr = std::shared_ptr<Frame>;

class IFrameQueue {
public:
    virtual ~IFrameQueue() = default;

    virtual void init(int capacity) = 0;
    virtual void clear() = 0;
    virtual void abort() = 0;
    virtual void flushForSeek() = 0;

    virtual bool push(DecodedFramePtr frame) = 0;
    virtual DecodedFramePtr pop(int timeoutMs = -1) = 0;
    virtual DecodedFramePtr peek() const = 0;

    virtual int size() const = 0;
    virtual int currentSerial() const = 0;
};

#endif // IFRAMEQUEUE_H
