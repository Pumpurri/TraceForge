#pragma once

#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "traceforge/v1/telemetry.pb.h"

namespace traceforge::generator {

struct SensorRates {
    std::uint32_t accelerometer_hz{50};
    std::uint32_t gyroscope_hz{50};
    std::uint32_t camera_hz{30};
    std::uint32_t gps_hz{10};
    std::uint32_t temperature_hz{1};
    std::uint32_t fault_monitor_hz{2};
};

struct FaultInjection {
    std::uint32_t drop_per_million{0};
    std::uint32_t duplicate_per_million{0};
    std::uint32_t reorder_per_million{0};
};

struct GeneratorConfig {
    std::uint64_t seed{1};
    std::chrono::nanoseconds duration{std::chrono::seconds{1}};
    std::chrono::nanoseconds max_jitter{0};
    std::int64_t clock_drift_ppm{0};
    SensorRates rates;
    FaultInjection faults;
};

struct GenerationStats {
    std::uint64_t generated_events{0};
    std::uint64_t dropped_events{0};
    std::uint64_t duplicated_events{0};
    std::uint64_t reordered_pairs{0};
};

struct GeneratedWorkload {
    std::vector<v1::TelemetryEvent> events;
    GenerationStats stats;
    std::uint64_t hash{0};
};

class WorkloadGenerator {
  public:
    explicit WorkloadGenerator(GeneratorConfig config);

    [[nodiscard]] GeneratedWorkload generate() const;

  private:
    GeneratorConfig config_;
};

[[nodiscard]] std::uint64_t
stable_event_hash(std::span<const v1::TelemetryEvent> events);
[[nodiscard]] std::string format_hash(std::uint64_t hash);
[[nodiscard]] std::string_view
payload_name(v1::TelemetryEvent::PayloadCase payload) noexcept;

}  // namespace traceforge::generator
