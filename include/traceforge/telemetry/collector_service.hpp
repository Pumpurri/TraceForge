#pragma once

#include <cstdint>

#include <grpcpp/support/server_callback.h>

#include "traceforge/v1/telemetry.grpc.pb.h"

namespace traceforge::telemetry {

struct CollectedEvent {
    v1::TelemetryEvent event;
    std::int64_t collector_arrival_timestamp_ns;
};

enum class SinkResult {
    accepted,
    overloaded,
    unavailable,
};

class TelemetrySink {
  public:
    virtual ~TelemetrySink() = default;
    virtual SinkResult try_accept(CollectedEvent event) = 0;
};

class CollectorService final : public v1::TelemetryCollector::CallbackService {
  public:
    explicit CollectorService(TelemetrySink& sink) noexcept;

    grpc::ServerReadReactor<v1::TelemetryEvent>*
    StreamTelemetry(grpc::CallbackServerContext* context,
                    v1::StreamSummary* response) override;

  private:
    TelemetrySink& sink_;
};

}  // namespace traceforge::telemetry
