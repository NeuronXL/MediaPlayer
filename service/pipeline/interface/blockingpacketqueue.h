#ifndef BLOCKINGPACKETQUEUE_H
#define BLOCKINGPACKETQUEUE_H

#include "ipacketqueue.h"

#include <condition_variable>
#include <mutex>
#include <vector>

class BlockingPacketQueue : public IPacketQueue {
public:
    explicit BlockingPacketQueue(int capacity = 16);

    void init(int capacity) override;
    void clear() override;
    void close() override;
    void abort() override;
    void flushForSeek() override;

    PacketQueuePushStatus push(EncodedPacketPtr packet) override;
    PacketPopResult pop(int timeoutMs = -1) override;
    EncodedPacketPtr peek() const override;

    int size() const override;
    int currentSerial() const override;
    bool isAborted() const override;
    bool isClosed() const override;

private:
    void clearLocked();
    void resetStateLocked();

private:
    mutable std::mutex mutex_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    std::vector<EncodedPacketPtr> queue_;

    int serial_ = 0;
    int rindex_ = 0;
    int windex_ = 0;
    int size_ = 0;
    int capacity_ = 16;
    bool aborted_ = false;
    bool closed_ = false;
};

#endif // BLOCKINGPACKETQUEUE_H
