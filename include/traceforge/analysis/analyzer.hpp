#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace traceforge::analysis {

enum class AnalysisStatus {
    complete,
    log_error,
};

struct ProducerStats {
    std::string producer_id;
    std::uint64_t records{0};
    std::uint64_t sequence_gaps{0};
    std::uint64_t non_increasing_sequences{0};
};

struct StreamStats {
    std::string producer_id;
    std::string payload;
    std::uint64_t records{0};
    std::optional<double> observed_rate_hz;
    std::optional<double> interarrival_p50_ms;
    std::optional<double> jitter_p95_ms;
    std::uint64_t source_timestamp_regressions{0};
};

struct DistributionStats {
    std::uint64_t samples{0};
    double p50_ms{0.0};
    double p95_ms{0.0};
    double p99_ms{0.0};
    double maximum_ms{0.0};
};

struct FaultStats {
    std::uint64_t warnings{0};
    std::uint64_t errors{0};
    std::uint64_t critical{0};
};

struct AnalysisReport {
    AnalysisStatus status{AnalysisStatus::log_error};
    std::uint64_t records{0};
    std::uint64_t file_bytes{0};
    std::optional<double> duration_seconds;
    std::vector<ProducerStats> producers;
    std::vector<StreamStats> streams;
    std::optional<DistributionStats> utc_arrival_delta;
    FaultStats faults;
    std::string message;
};

[[nodiscard]] AnalysisReport
analyze_log(const std::filesystem::path& path,
            std::size_t maximum_records = 1'000'000);
[[nodiscard]] std::string_view status_name(AnalysisStatus status) noexcept;

}  // namespace traceforge::analysis
