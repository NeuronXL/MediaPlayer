#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

#include "../common/packet.h"

#define MAX_PACKETQUEUE_SIZE 16

class PacketQueue {
    public:
    explicit PacketQueue(int capacity = MAX_PACKETQUEUE_SIZE) : serial_(0), capacity_(capacity) {}

    ~PacketQueue() {
        clear();
    }

    void clear() {
        abort();
        std::unique_lock<std::mutex> lock(mutex_);
        while (size_) {
            delete queue_[rindex_];
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
            for (Packet*& packet : queue_) {
                packet = nullptr;
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

    Packet* pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [&] { return aborted_ || size_ > 0; });
        if (aborted_)
            return nullptr;

        --size_;
        int preIndex = rindex_;
        rindex_ = (rindex_ + 1 + capacity_) % capacity_;
        Packet* packet = queue_[preIndex];
        queue_[preIndex] = nullptr;
        notFull_.notify_all();
        return packet;
    }

    bool push(Packet* packet) {
        std::unique_lock<std::mutex> lock(mutex_);
        const int expectedSerial = serial_;
        notFull_.wait(lock, [&] { return aborted_ || size_ < capacity_ || serial_ != expectedSerial; });
        if (aborted_)
            return false;
        if (serial_ != expectedSerial)
            return false;
        ++size_;
        if (packet != nullptr) {
            packet->serial = serial_;
        }
        queue_[windex_] = packet;
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
            delete queue_[rindex_];
            queue_[rindex_] = nullptr;
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
    std::vector<Packet*> queue_;

    int serial_;
    int rindex_;
    int windex_;
    int size_;
    int capacity_;
    bool aborted_;
};
#endif // PACKETQUEUE_H
