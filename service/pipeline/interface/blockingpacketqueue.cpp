#include "blockingpacketqueue.h"

#include <chrono>
#include <utility>

BlockingPacketQueue::BlockingPacketQueue(int capacity) {
    init(capacity);
}

void BlockingPacketQueue::init(int capacity) {
    std::unique_lock<std::mutex> lock(mutex_);
    capacity_ = capacity > 0 ? capacity : 1;
    queue_.assign(static_cast<size_t>(capacity_), nullptr);
    resetStateLocked();
}

void BlockingPacketQueue::clear() {
    std::unique_lock<std::mutex> lock(mutex_);
    clearLocked();
}

void BlockingPacketQueue::close() {
    std::unique_lock<std::mutex> lock(mutex_);
    closed_ = true;
    notEmpty_.notify_all();
    notFull_.notify_all();
}

void BlockingPacketQueue::abort() {
    std::unique_lock<std::mutex> lock(mutex_);
    aborted_ = true;
    notEmpty_.notify_all();
    notFull_.notify_all();
}

void BlockingPacketQueue::flushForSeek() {
    std::unique_lock<std::mutex> lock(mutex_);
    clearLocked();
    ++serial_;
    notEmpty_.notify_all();
    notFull_.notify_all();
}

PacketQueuePushStatus BlockingPacketQueue::push(EncodedPacketPtr packet) {
    std::unique_lock<std::mutex> lock(mutex_);
    notFull_.wait(lock, [&] { return aborted_ || closed_ || size_ < capacity_; });
    if (aborted_) {
        return PacketQueuePushStatus::Aborted;
    }
    if (closed_) {
        return PacketQueuePushStatus::Closed;
    }

    if (packet) {
        packet->serial = serial_;
    }
    queue_[static_cast<size_t>(windex_)] = std::move(packet);
    windex_ = (windex_ + 1) % capacity_;
    ++size_;
    notEmpty_.notify_one();
    return PacketQueuePushStatus::Ok;
}

PacketPopResult BlockingPacketQueue::pop(int timeoutMs) {
    std::unique_lock<std::mutex> lock(mutex_);

    const auto canPop = [&] { return aborted_ || size_ > 0 || closed_; };

    if (timeoutMs < 0) {
        notEmpty_.wait(lock, canPop);
    } else {
        const bool ready = notEmpty_.wait_for(lock, std::chrono::milliseconds(timeoutMs), canPop);
        if (!ready) {
            return { PacketQueuePopStatus::Timeout, nullptr, serial_ };
        }
    }

    if (aborted_) {
        return { PacketQueuePopStatus::Aborted, nullptr, serial_ };
    }
    if (size_ == 0 && closed_) {
        return { PacketQueuePopStatus::Closed, nullptr, serial_ };
    }
    if (size_ == 0) {
        return { PacketQueuePopStatus::Timeout, nullptr, serial_ };
    }

    EncodedPacketPtr packet = std::move(queue_[static_cast<size_t>(rindex_)]);
    queue_[static_cast<size_t>(rindex_)].reset();
    rindex_ = (rindex_ + 1) % capacity_;
    --size_;
    notFull_.notify_one();
    return { PacketQueuePopStatus::Ok, std::move(packet), serial_ };
}

EncodedPacketPtr BlockingPacketQueue::peek() const {
    std::unique_lock<std::mutex> lock(mutex_);
    if (size_ <= 0) {
        return nullptr;
    }
    return queue_[static_cast<size_t>(rindex_)];
}

int BlockingPacketQueue::size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return size_;
}

int BlockingPacketQueue::currentSerial() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return serial_;
}

bool BlockingPacketQueue::isAborted() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return aborted_;
}

bool BlockingPacketQueue::isClosed() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return closed_;
}

void BlockingPacketQueue::clearLocked() {
    for (int i = 0; i < size_; ++i) {
        const int index = (rindex_ + i) % capacity_;
        queue_[static_cast<size_t>(index)].reset();
    }
    rindex_ = 0;
    windex_ = 0;
    size_ = 0;
}

void BlockingPacketQueue::resetStateLocked() {
    rindex_ = 0;
    windex_ = 0;
    size_ = 0;
    aborted_ = false;
    closed_ = false;
    ++serial_;
}
