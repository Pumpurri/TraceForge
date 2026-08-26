#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "traceforge/concurrency/bounded_queue.hpp"

int main() {
    using traceforge::concurrency::BoundedQueue;
    using traceforge::concurrency::QueuePushResult;

    BoundedQueue<int> queue{2};
    if (queue.try_push(10) != QueuePushResult::accepted ||
        queue.try_push(20) != QueuePushResult::accepted ||
        queue.try_push(30) != QueuePushResult::full) {
        std::cerr << "Bounded queue did not reject an over-capacity push\n";
        return 1;
    }

    const auto first = queue.wait_pop();
    if (!first || *first != 10 ||
        queue.try_push(30) != QueuePushResult::accepted) {
        std::cerr << "Bounded queue did not preserve FIFO order\n";
        return 1;
    }

    queue.close();
    const auto second = queue.wait_pop();
    const auto third = queue.wait_pop();
    const auto finished = queue.wait_pop();
    if (!second || *second != 20 || !third || *third != 30 || finished) {
        std::cerr << "Closing the queue did not drain accepted items\n";
        return 1;
    }

    const auto initial_stats = queue.stats();
    if (initial_stats.capacity != 2 || initial_stats.current_size != 0 ||
        initial_stats.high_watermark != 2 ||
        initial_stats.accepted_items != 3 ||
        initial_stats.rejected_items != 1 || !initial_stats.closed) {
        std::cerr << "Bounded queue statistics are incorrect\n";
        return 1;
    }

    constexpr int kProducerCount = 4;
    constexpr int kItemsPerProducer = 500;
    constexpr int kTotalItems = kProducerCount * kItemsPerProducer;
    BoundedQueue<int> concurrent_queue{32};
    std::vector<std::uint8_t> received(static_cast<std::size_t>(kTotalItems),
                                       0);

    std::thread consumer{[&] {
        while (auto value = concurrent_queue.wait_pop()) {
            if (*value >= 0 && *value < kTotalItems) {
                received[static_cast<std::size_t>(*value)] = 1;
            }
        }
    }};

    std::vector<std::thread> producers;
    for (int producer = 0; producer < kProducerCount; ++producer) {
        producers.emplace_back([&, producer] {
            for (int offset = 0; offset < kItemsPerProducer; ++offset) {
                const int value = producer * kItemsPerProducer + offset;
                while (concurrent_queue.try_push(value) ==
                       QueuePushResult::full) {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (auto& producer : producers) {
        producer.join();
    }
    concurrent_queue.close();
    consumer.join();

    for (const auto was_received : received) {
        if (was_received == 0) {
            std::cerr << "Concurrent queue lost an accepted item\n";
            return 1;
        }
    }
    const auto concurrent_stats = concurrent_queue.stats();
    if (concurrent_stats.accepted_items != kTotalItems ||
        concurrent_stats.current_size != 0 ||
        concurrent_stats.high_watermark > concurrent_stats.capacity) {
        std::cerr << "Concurrent queue exceeded its memory bound\n";
        return 1;
    }

    return 0;
}
