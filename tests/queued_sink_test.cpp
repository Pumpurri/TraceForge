#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <utility>
#include <vector>

#include "traceforge/telemetry/queued_sink.hpp"

namespace {

traceforge::telemetry::CollectedEvent
make_event(std::uint64_t sequence_number) {
    traceforge::telemetry::CollectedEvent collected;
    collected.event.set_sequence_number(sequence_number);
    collected.collector_arrival_timestamp_ns = 1;
    return collected;
}

class BlockingConsumer final : public traceforge::telemetry::TelemetryConsumer {
  public:
    bool consume(traceforge::telemetry::CollectedEvent event) override {
        std::unique_lock lock{mutex_};
        entered_ = true;
        entered_condition_.notify_one();
        release_condition_.wait(lock, [this] { return released_; });
        sequences_.push_back(event.event.sequence_number());
        return true;
    }

    void wait_until_entered() {
        std::unique_lock lock{mutex_};
        entered_condition_.wait(lock, [this] { return entered_; });
    }

    void release() {
        {
            std::lock_guard lock{mutex_};
            released_ = true;
        }
        release_condition_.notify_one();
    }

    [[nodiscard]] std::vector<std::uint64_t> sequences() const {
        std::lock_guard lock{mutex_};
        return sequences_;
    }

  private:
    mutable std::mutex mutex_;
    std::condition_variable entered_condition_;
    std::condition_variable release_condition_;
    std::vector<std::uint64_t> sequences_;
    bool entered_{false};
    bool released_{false};
};

}  // namespace

int main() {
    using traceforge::telemetry::QueuedTelemetrySink;
    using traceforge::telemetry::SinkResult;

    BlockingConsumer consumer;
    QueuedTelemetrySink sink{2, consumer};
    if (sink.try_accept(make_event(1)) != SinkResult::accepted) {
        std::cerr << "Sink rejected its first event\n";
        return 1;
    }
    consumer.wait_until_entered();

    if (sink.try_accept(make_event(2)) != SinkResult::accepted ||
        sink.try_accept(make_event(3)) != SinkResult::accepted ||
        sink.try_accept(make_event(4)) != SinkResult::overloaded) {
        std::cerr << "Slow consumer did not trigger explicit overload\n";
        consumer.release();
        return 1;
    }

    const auto overloaded_stats = sink.stats();
    if (overloaded_stats.queue.current_size != 2 ||
        overloaded_stats.queue.high_watermark != 2 ||
        overloaded_stats.queue.rejected_items != 1) {
        std::cerr << "Overload statistics are incorrect\n";
        consumer.release();
        return 1;
    }

    consumer.release();
    sink.shutdown();
    const auto final_stats = sink.stats();
    const auto sequences = consumer.sequences();
    if (sequences != std::vector<std::uint64_t>{1, 2, 3} ||
        final_stats.consumed_events != 3 ||
        final_stats.queue.high_watermark > final_stats.queue.capacity ||
        final_stats.accepting_events ||
        sink.try_accept(make_event(5)) != SinkResult::unavailable) {
        std::cerr << "Queued sink did not drain or shut down correctly\n";
        return 1;
    }

    return 0;
}
