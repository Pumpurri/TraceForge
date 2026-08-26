#include "traceforge/telemetry/queued_sink.hpp"

#include <utility>

namespace traceforge::telemetry {

QueuedTelemetrySink::QueuedTelemetrySink(std::size_t capacity,
                                         TelemetryConsumer& consumer)
    : queue_(capacity), consumer_(consumer),
      consumer_thread_([this] { consume_loop(); }) {}

QueuedTelemetrySink::~QueuedTelemetrySink() { shutdown(); }

SinkResult QueuedTelemetrySink::try_accept(CollectedEvent event) {
    if (!accepting_events_.load(std::memory_order_relaxed)) {
        return SinkResult::unavailable;
    }

    switch (queue_.try_push(std::move(event))) {
    case concurrency::QueuePushResult::accepted:
        return SinkResult::accepted;
    case concurrency::QueuePushResult::full:
        return SinkResult::overloaded;
    case concurrency::QueuePushResult::closed:
        return SinkResult::unavailable;
    }
    return SinkResult::unavailable;
}

void QueuedTelemetrySink::shutdown() {
    accepting_events_.store(false, std::memory_order_relaxed);
    queue_.close();
    if (consumer_thread_.joinable()) {
        consumer_thread_.join();
    }
}

IngestionStats QueuedTelemetrySink::stats() const {
    return {
        .queue = queue_.stats(),
        .consumed_events = consumed_events_.load(std::memory_order_relaxed),
        .consumer_failures = consumer_failures_.load(std::memory_order_relaxed),
        .accepting_events = accepting_events_.load(std::memory_order_relaxed),
    };
}

void QueuedTelemetrySink::consume_loop() {
    while (auto event = queue_.wait_pop()) {
        if (!consumer_.consume(std::move(*event))) {
            consumer_failures_.fetch_add(1, std::memory_order_relaxed);
            accepting_events_.store(false, std::memory_order_relaxed);
            queue_.close();
            return;
        }
        consumed_events_.fetch_add(1, std::memory_order_relaxed);
    }
}

}  // namespace traceforge::telemetry
