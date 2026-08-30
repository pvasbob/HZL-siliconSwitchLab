#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace silicon_switch::asic {

enum class QueueEnqueueResult {
    enqueued,
    full,
    closed,
};

template <typename Value>
class BoundedQueue {
public:
    explicit BoundedQueue(const std::size_t capacity) : capacity_{capacity} {
        if (capacity_ == 0U) {
            throw std::invalid_argument{"queue capacity must be positive"};
        }
    }

    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    [[nodiscard]] QueueEnqueueResult try_enqueue(Value value) {
        std::lock_guard<std::mutex> lock{mutex_};
        if (closed_) {
            return QueueEnqueueResult::closed;
        }
        if (queue_.size() >= capacity_) {
            return QueueEnqueueResult::full;
        }
        queue_.push_back(std::move(value));
        available_.notify_one();
        return QueueEnqueueResult::enqueued;
    }

    [[nodiscard]] std::optional<Value> try_dequeue() {
        std::lock_guard<std::mutex> lock{mutex_};
        return pop_front();
    }

    [[nodiscard]] std::optional<Value> wait_dequeue() {
        std::unique_lock<std::mutex> lock{mutex_};
        available_.wait(lock, [this] { return closed_ || !queue_.empty(); });
        return pop_front();
    }

    [[nodiscard]] bool close() {
        std::lock_guard<std::mutex> lock{mutex_};
        if (closed_) {
            return false;
        }
        closed_ = true;
        available_.notify_all();
        return true;
    }

    [[nodiscard]] bool closed() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return closed_;
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return queue_.empty();
    }

    [[nodiscard]] bool full() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return queue_.size() >= capacity_;
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return queue_.size();
    }

    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return capacity_;
    }

private:
    [[nodiscard]] std::optional<Value> pop_front() {
        if (queue_.empty()) {
            return std::nullopt;
        }
        Value value = std::move(queue_.front());
        queue_.pop_front();
        return value;
    }

    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable available_;
    std::deque<Value> queue_;
    bool closed_{false};
};

}  // namespace silicon_switch::asic
