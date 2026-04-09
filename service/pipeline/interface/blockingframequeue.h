#ifndef BLOCKINGFRAMEQUEUE_H
#define BLOCKINGFRAMEQUEUE_H

#include "iframequeue.h"

#include <condition_variable>
#include <mutex>
#include <vector>

class BlockingFrameQueue : public IFrameQueue {
public:
    explicit BlockingFrameQueue(int capacity = 16);

    void init(int capacity) override;
    void clear() override;
    void abort() override;
    void flushForSeek() override;

    bool push(DecodedFramePtr frame) override;
    DecodedFramePtr pop(int timeoutMs = -1) override;
    DecodedFramePtr peek() const override;

    int size() const override;
    int currentSerial() const override;

private:
    void clearLocked();
    void resetStateLocked();

private:
    mutable std::mutex mutex_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    std::vector<DecodedFramePtr> queue_;

    int serial_ = 0;
    int rindex_ = 0;
    int windex_ = 0;
    int size_ = 0;
    int capacity_ = 16;
    bool aborted_ = false;
};

#endif // BLOCKINGFRAMEQUEUE_H
