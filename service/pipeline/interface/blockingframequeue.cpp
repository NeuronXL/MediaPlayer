#include "blockingframequeue.h"

#include <chrono>

BlockingFrameQueue::BlockingFrameQueue(int capacity) {
    init(capacity);
}

void BlockingFrameQueue::init(int capacity) {
    std::unique_lock<std::mutex> lock(mutex_);
    capacity_ = capacity > 0 ? capacity : 1;
    queue_.assign(static_cast<size_t>(capacity_), nullptr);
    resetStateLocked();
}

void BlockingFrameQueue::clear() {
    std::unique_lock<std::mutex> lock(mutex_);
    clearLocked();
    resetStateLocked();
}

void BlockingFrameQueue::abort() {
    std::unique_lock<std::mutex> lock(mutex_);
    aborted_ = true;
    notEmpty_.notify_all();
    notFull_.notify_all();
}

void BlockingFrameQueue::flushForSeek() {
    std::unique_lock<std::mutex> lock(mutex_);
    clearLocked();
    ++serial_;
    notEmpty_.notify_all();
    notFull_.notify_all();
}

bool BlockingFrameQueue::push(DecodedFramePtr frame) {
    std::unique_lock<std::mutex> lock(mutex_);
    const int expectedSerial = serial_;
    notFull_.wait(lock, [&] { return aborted_ || size_ < capacity_ || serial_ != expectedSerial; });
    if (aborted_) {
        return false;
    }
    if (serial_ != expectedSerial) {
        return false;
    }

    queue_[static_cast<size_t>(windex_)] = std::move(frame);
    windex_ = (windex_ + 1) % capacity_;
    ++size_;
    notEmpty_.notify_one();
    return true;
}

DecodedFramePtr BlockingFrameQueue::pop(int timeoutMs) {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto canPop = [&] { return aborted_ || size_ > 0; };
    if (timeoutMs < 0) {
        notEmpty_.wait(lock, canPop);
    } else {
        const bool ready = notEmpty_.wait_for(lock, std::chrono::milliseconds(timeoutMs), canPop);
        if (!ready) {
            return nullptr;
        }
    }
    if (aborted_ || size_ == 0) {
        return nullptr;
    }

    DecodedFramePtr frame = std::move(queue_[static_cast<size_t>(rindex_)]);
    queue_[static_cast<size_t>(rindex_)].reset();
    rindex_ = (rindex_ + 1) % capacity_;
    --size_;
    notFull_.notify_one();
    return frame;
}

DecodedFramePtr BlockingFrameQueue::peek() const {
    std::unique_lock<std::mutex> lock(mutex_);
    if (size_ <= 0) {
        return nullptr;
    }
    return queue_[static_cast<size_t>(rindex_)];
}

int BlockingFrameQueue::size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return size_;
}

int BlockingFrameQueue::currentSerial() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return serial_;
}

void BlockingFrameQueue::clearLocked() {
    for (int i = 0; i < size_; ++i) {
        const int idx = (rindex_ + i) % capacity_;
        queue_[static_cast<size_t>(idx)].reset();
    }
    rindex_ = 0;
    windex_ = 0;
    size_ = 0;
}

void BlockingFrameQueue::resetStateLocked() {
    rindex_ = 0;
    windex_ = 0;
    size_ = 0;
    aborted_ = false;
    ++serial_;
}
