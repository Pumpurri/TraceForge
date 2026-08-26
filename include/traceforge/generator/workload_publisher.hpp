#pragma once

#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "traceforge/v1/telemetry.pb.h"

namespace traceforge::generator {

struct ProducerPublishResult {
    std::string producer_id;
    std::uint64_t attempted_events{0};
    std::uint64_t written_events{0};
    std::uint64_t accepted_events{0};
    std::uint64_t sequence_gaps{0};
    int grpc_status_code{0};
    std::string error_message;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct PublishReport {
    std::vector<ProducerPublishResult> producers;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] std::uint64_t attempted_events() const noexcept;
    [[nodiscard]] std::uint64_t accepted_events() const noexcept;
};

struct WorkloadPublisherConfig {
    std::string target{"127.0.0.1:50051"};
    std::chrono::milliseconds connect_timeout{std::chrono::seconds{5}};
};

class WorkloadPublisher {
  public:
    explicit WorkloadPublisher(WorkloadPublisherConfig config);

    [[nodiscard]] PublishReport
    publish(std::span<const v1::TelemetryEvent> events) const;

  private:
    WorkloadPublisherConfig config_;
};

}  // namespace traceforge::generator
