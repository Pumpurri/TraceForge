#pragma once

#include <cstdint>
#include <string>

#include "traceforge/v1/telemetry.pb.h"

namespace traceforge::telemetry {

inline constexpr std::uint32_t kTelemetrySchemaVersion = 1;

struct ValidationResult {
    bool accepted;
    std::string error;
};

class StreamValidator {
  public:
    [[nodiscard]] ValidationResult accept(const v1::TelemetryEvent& event);
    void populate_summary(v1::StreamSummary& summary) const;

  private:
    std::string producer_id_;
    std::uint64_t accepted_events_{0};
    std::uint64_t first_sequence_number_{0};
    std::uint64_t last_sequence_number_{0};
    std::uint64_t sequence_gaps_{0};
};

}  // namespace traceforge::telemetry
