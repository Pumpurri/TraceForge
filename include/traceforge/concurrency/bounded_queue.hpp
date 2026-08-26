#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace traceforge::concurrency {

enum class QueuePushResult {
    accepted,
    full,
    closed,
};

struct BoundedQueueStats {
    std::size_t capacity{0};
    std::size_t current_size{0};
    std::size_t high_watermark{0};
    std::uint64_t accepted_items{0};
    std::uint64_t rejected_items{0};
    bool closed{false};
};

template <typename Item> class BoundedQueue {
  public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("queue capacity must be positive");
        }
    }

    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    [[nodiscard]] QueuePushResult try_push(Item item) {
        {
            std::lock_guard lock{mutex_};
            if (closed_) {
                ++rejected_items_;
                return QueuePushResult::closed;
            }
            if (items_.size() == capacity_) {
                ++rejected_items_;
                return QueuePushResult::full;
            }

            items_.push_back(std::move(item));
            ++accepted_items_;
            if (items_.size() > high_watermark_) {
                high_watermark_ = items_.size();
            }
        }
        item_available_.notify_one();
        return QueuePushResult::accepted;
    }

    [[nodiscard]] std::optional<Item> wait_pop() {
        std::unique_lock lock{mutex_};
        item_available_.wait(lock,
                             [this] { return closed_ || !items_.empty(); });
        if (items_.empty()) {
            return std::nullopt;
        }

        Item item = std::move(items_.front());
        items_.pop_front();
        return item;
    }

    void close() {
        {
            std::lock_guard lock{mutex_};
            closed_ = true;
        }
        item_available_.notify_all();
    }

    [[nodiscard]] BoundedQueueStats stats() const {
        std::lock_guard lock{mutex_};
        return {
            .capacity = capacity_,
            .current_size = items_.size(),
            .high_watermark = high_watermark_,
            .accepted_items = accepted_items_,
            .rejected_items = rejected_items_,
            .closed = closed_,
        };
    }

  private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable item_available_;
    std::deque<Item> items_;
    std::size_t high_watermark_{0};
    std::uint64_t accepted_items_{0};
    std::uint64_t rejected_items_{0};
    bool closed_{false};
};

}  // namespace traceforge::concurrency
