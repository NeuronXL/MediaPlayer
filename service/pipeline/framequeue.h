#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include "../common/frame.h"

#define MAX_FRAME_CAPACITY 16

using FramePtr = std::shared_ptr<Frame>;

class FrameQueue {
    public:
    explicit FrameQueue(bool cacheEnable = true, int capacity = MAX_FRAME_CAPACITY)
        : capacity_(capacity), cacheEnable_(cacheEnable), serial_(0) {
        init();
    }

    ~FrameQueue() {
        clear();
    }

    void clear() {
        abort();
        std::unique_lock<std::mutex> lock(mutex_);
        while (size_) {
            queue_[rindex_].reset();
            rindex_ = (rindex_ + 1) % capacity_;
            --size_;
        }
        resetStateLocked();
    }

    void init() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (static_cast<int>(queue_.size()) != capacity_) {
            queue_.assign(capacity_, nullptr);
        } else {
            for (FramePtr& frame : queue_) {
                frame.reset();
            }
        }
        resetStateLocked();
    }

    int size() {
        std::unique_lock<std::mutex> lock(mutex_);
        return size_;
    }

    int currentSerial() {
        std::unique_lock<std::mutex> lock(mutex_);
        return serial_;
    }

    FramePtr peekLast() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (size_ <= 0) {
            return {};
        }
        return queue_[(rindex_ - 1 + capacity_) % capacity_];
    }

    FramePtr peekNext() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (size_)
            return queue_[rindex_];
        return {};
    }

    FramePtr pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [&] { return aborted_ || size_ > 0; });
        if (aborted_)
            return {};

        FramePtr popped = queue_[rindex_];
        queue_[rindex_].reset();
        --size_;
        rindex_ = (rindex_ + 1) % capacity_;
        notFull_.notify_all();
        return popped;
    }

    bool push(FramePtr frame) {
        std::unique_lock<std::mutex> lock(mutex_);
        const int expectedSerial = serial_;
        notFull_.wait(lock, [&] { return aborted_ || size_ < capacity_ || serial_ != expectedSerial; });
        if (aborted_)
            return false;
        if (serial_ != expectedSerial)
            return false;
        ++size_;
        queue_[windex_] = frame;
        windex_ = (windex_ + 1 + capacity_) % capacity_;
        notEmpty_.notify_one();
        return true;
    }

    void abort() {
        std::unique_lock<std::mutex> lock(mutex_);
        aborted_ = true;
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

    void flushForSeek() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (size_) {
            queue_[rindex_].reset();
            rindex_ = (rindex_ + 1) % capacity_;
            --size_;
        }
        rindex_ = 0;
        windex_ = 0;
        ++serial_;
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

    private:
    void resetStateLocked() {
        rindex_ = 0;
        windex_ = 0;
        aborted_ = false;
        size_ = 0;
        ++serial_;
    }

    private:
    mutable std::mutex mutex_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    std::vector<FramePtr> queue_;

    int serial_;
    int rindex_;
    int windex_;
    int size_;
    int capacity_;
    bool aborted_;
    bool cacheEnable_;
};
#endif // FRAMEQUEUE_H
