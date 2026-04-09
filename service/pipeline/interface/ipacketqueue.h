#ifndef IPACKETQUEUE_H
#define IPACKETQUEUE_H

#include "../../common/packet.h"

enum class PacketQueuePushStatus {
    Ok,
    Closed,
    Aborted
};

enum class PacketQueuePopStatus {
    Ok,
    Closed,
    Aborted,
    Timeout
};

struct PacketPopResult {
    PacketQueuePopStatus status = PacketQueuePopStatus::Timeout;
    EncodedPacketPtr packet;
    int serial = 0;
};

class IPacketQueue {
public:
    virtual ~IPacketQueue() = default;

    virtual void init(int capacity) = 0;
    virtual void clear() = 0;
    virtual void close() = 0;
    virtual void abort() = 0;
    virtual void flushForSeek() = 0;

    virtual PacketQueuePushStatus push(EncodedPacketPtr packet) = 0;
    virtual PacketPopResult pop(int timeoutMs = -1) = 0;
    virtual EncodedPacketPtr peek() const = 0;

    virtual int size() const = 0;
    virtual int currentSerial() const = 0;
    virtual bool isAborted() const = 0;
    virtual bool isClosed() const = 0;
};

#endif // IPACKETQUEUE_H
