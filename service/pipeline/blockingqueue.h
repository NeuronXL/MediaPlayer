#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <type_traits>
#include <utility>

#include <QtGlobal>

#include "../common/timestampeditem.h"

template <typename T> class BlockingQueue
{
    static_assert(std::is_base_of<TimestampedItem, T>::value,
                  "BlockingQueue<T> requires T to inherit from TimestampedItem.");

  public:
    // maxBufferedDurationMs <= 0 means unlimited capacity.
    explicit BlockingQueue(qint64 maxBufferedDurationMs = 0)
        : maxBufferedDurationMs_(maxBufferedDurationMs)
        , closed_(false)
    {
    }

    void setMaxBufferedDurationMs(qint64 maxBufferedDurationMs)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        maxBufferedDurationMs_ = maxBufferedDurationMs;
        notFull_.notify_all();
    }

    void setCapacity(qint64 maxBufferedDurationMs)
    {
        setMaxBufferedDurationMs(maxBufferedDurationMs);
    }

    qint64 maxBufferedDurationMs() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return maxBufferedDurationMs_;
    }

    qint64 capacity() const
    {
        return maxBufferedDurationMs();
    }

    qint64 bufferedSpanMs() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return bufferedSpanMs_;
    }

    bool push(const T& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [&] { return closed_ || !isFullLocked(item); });
        if (closed_) {
            return false;
        }
        queue_.push_back(item);
        updateBufferedSpanLocked();
        notEmpty_.notify_one();
        return true;
    }

    bool push(T&& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [&] { return closed_ || !isFullLocked(item); });
        if (closed_) {
            return false;
        }
        queue_.push_back(std::move(item));
        updateBufferedSpanLocked();
        notEmpty_.notify_one();
        return true;
    }

    bool tryPush(const T& item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || isFullLocked(item)) {
            return false;
        }
        queue_.push_back(item);
        updateBufferedSpanLocked();
        notEmpty_.notify_one();
        return true;
    }

    bool tryPush(T&& item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || isFullLocked(item)) {
            return false;
        }
        queue_.push_back(std::move(item));
        updateBufferedSpanLocked();
        notEmpty_.notify_one();
        return true;
    }

    bool pop(T& out)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [&] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) {
            return false;
        }

        out = std::move(queue_.front());
        queue_.pop_front();
        updateBufferedSpanLocked();
        notFull_.notify_all();
        return true;
    }

    bool tryPop(T& out)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }

        out = std::move(queue_.front());
        queue_.pop_front();
        updateBufferedSpanLocked();
        notFull_.notify_one();
        return true;
    }

    void open()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = false;
        notFull_.notify_all();
        notEmpty_.notify_all();
    }

    void close()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        notFull_.notify_all();
        notEmpty_.notify_all();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
        bufferedSpanMs_ = 0;
        notFull_.notify_all();
    }

    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

  private:
    bool isFullLocked(const T& incoming) const
    {
        if (maxBufferedDurationMs_ <= 0 || queue_.empty()) {
            return false;
        }

        const qint64 headPts = queue_.front().ptsMs;
        const qint64 tailPts = incoming.ptsMs >= 0 ? incoming.ptsMs : queue_.back().ptsMs;
        if (headPts < 0 || tailPts < 0) {
            return false;
        }

        const qint64 nextSpan = qMax<qint64>(0, tailPts - headPts);
        return nextSpan >= maxBufferedDurationMs_;
    }

    void updateBufferedSpanLocked()
    {
        if (queue_.empty()) {
            bufferedSpanMs_ = 0;
            return;
        }

        const qint64 headPts = queue_.front().ptsMs;
        const qint64 tailPts = queue_.back().ptsMs;
        if (headPts < 0 || tailPts < 0) {
            bufferedSpanMs_ = 0;
            return;
        }

        bufferedSpanMs_ = qMax<qint64>(0, tailPts - headPts);
    }

    qint64 maxBufferedDurationMs_;
    bool closed_;
    qint64 bufferedSpanMs_ = 0;
    mutable std::mutex mutex_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    std::deque<T> queue_;
};

#endif // BLOCKINGQUEUE_H
