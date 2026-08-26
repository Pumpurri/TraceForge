#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>

#include "traceforge/concurrency/bounded_queue.hpp"
#include "traceforge/telemetry/collector_service.hpp"

namespace traceforge::telemetry {

class TelemetryConsumer {
  public:
    virtual ~TelemetryConsumer() = default;

    // Returning false permanently stops this ingestion pipeline.
    virtual bool consume(CollectedEvent event) = 0;
};

struct IngestionStats {
    concurrency::BoundedQueueStats queue;
    std::uint64_t consumed_events{0};
    std::uint64_t consumer_failures{0};
    bool accepting_events{false};
};

class QueuedTelemetrySink final : public TelemetrySink {
  public:
    QueuedTelemetrySink(std::size_t capacity, TelemetryConsumer& consumer);
    ~QueuedTelemetrySink() override;

    QueuedTelemetrySink(const QueuedTelemetrySink&) = delete;
    QueuedTelemetrySink& operator=(const QueuedTelemetrySink&) = delete;

    SinkResult try_accept(CollectedEvent event) override;
    void shutdown();
    [[nodiscard]] IngestionStats stats() const;

  private:
    void consume_loop();

    concurrency::BoundedQueue<CollectedEvent> queue_;
    TelemetryConsumer& consumer_;
    std::thread consumer_thread_;
    std::atomic<std::uint64_t> consumed_events_{0};
    std::atomic<std::uint64_t> consumer_failures_{0};
    std::atomic<bool> accepting_events_{true};
};

}  // namespace traceforge::telemetry
