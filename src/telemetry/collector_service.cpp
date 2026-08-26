#include "traceforge/telemetry/collector_service.hpp"

#include <chrono>
#include <cstdint>
#include <utility>

#include <grpcpp/support/status.h>

#include "traceforge/telemetry/validation.hpp"

namespace traceforge::telemetry {
namespace {

[[nodiscard]] std::int64_t unix_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

class StreamReactor final : public grpc::ServerReadReactor<v1::TelemetryEvent> {
  public:
    StreamReactor(TelemetrySink& sink, v1::StreamSummary& response)
        : sink_(sink), response_(response) {
        StartRead(&event_);
    }

    void OnReadDone(bool ok) override {
        if (!ok) {
            validator_.populate_summary(response_);
            Finish(grpc::Status::OK);
            return;
        }

        const auto validation = validator_.accept(event_);
        if (!validation.accepted) {
            Finish({grpc::StatusCode::INVALID_ARGUMENT, validation.error});
            return;
        }

        CollectedEvent collected{
            .event = std::move(event_),
            .collector_arrival_timestamp_ns = unix_time_ns(),
        };
        if (sink_.try_accept(std::move(collected)) == SinkResult::overloaded) {
            Finish({grpc::StatusCode::RESOURCE_EXHAUSTED,
                    "telemetry sink is overloaded"});
            return;
        }

        event_.Clear();
        StartRead(&event_);
    }

    void OnDone() override { delete this; }

  private:
    TelemetrySink& sink_;
    v1::StreamSummary& response_;
    StreamValidator validator_;
    v1::TelemetryEvent event_;
};

}  // namespace

CollectorService::CollectorService(TelemetrySink& sink) noexcept : sink_(sink) {}

grpc::ServerReadReactor<v1::TelemetryEvent>* CollectorService::StreamTelemetry(
    grpc::CallbackServerContext* context,
    v1::StreamSummary* response) {
    static_cast<void>(context);
    return new StreamReactor{sink_, *response};
}

}  // namespace traceforge::telemetry
