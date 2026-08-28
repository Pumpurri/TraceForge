#include <atomic>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

#include "traceforge/analysis/analyzer.hpp"
#include "traceforge/recording/log.hpp"
#include "traceforge/telemetry/validation.hpp"
#include "traceforge/v1/telemetry.pb.h"

namespace {

class TemporaryLog {
  public:
    explicit TemporaryLog(std::string_view label) {
        static std::atomic<std::uint64_t> next_id{0};
        path_ = std::filesystem::temp_directory_path() /
                ("traceforge_analysis_" + std::string{label} + "_" +
                 std::to_string(next_id.fetch_add(1)) + ".tflog");
    }

    ~TemporaryLog() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

traceforge::v1::TelemetryEvent event(std::string_view producer,
                                     std::uint64_t sequence,
                                     std::int64_t source_timestamp_ns,
                                     bool accelerometer) {
    traceforge::v1::TelemetryEvent result;
    result.set_schema_version(traceforge::telemetry::kTelemetrySchemaVersion);
    result.set_producer_id(producer);
    result.set_sequence_number(sequence);
    result.set_source_timestamp_ns(source_timestamp_ns);
    result.set_source_clock(traceforge::v1::CLOCK_DOMAIN_UTC);
    if (accelerometer) {
        result.mutable_accelerometer()->set_z(9.81);
    } else {
        result.mutable_gyroscope()->set_z(0.1);
    }
    return result;
}

traceforge::v1::TelemetryEvent fault(std::uint64_t sequence,
                                     std::int64_t source_timestamp_ns,
                                     traceforge::v1::FaultEvent::Severity severity,
                                     traceforge::v1::ClockDomain clock) {
    auto result = event("fault-monitor", sequence, source_timestamp_ns, true);
    result.set_source_clock(clock);
    result.clear_accelerometer();
    result.mutable_fault()->set_severity(severity);
    result.mutable_fault()->set_code(100);
    result.mutable_fault()->set_message("test fault");
    return result;
}

bool near(double actual, double expected, double tolerance = 0.001) {
    return std::abs(actual - expected) <= tolerance;
}

}  // namespace

int main() {
    TemporaryLog log{"metrics"};
    {
        traceforge::recording::LogWriter writer{
            log.path(), {.flush_every_records = 1, .created_unix_ns = 1}};
        const auto append = [&writer](const auto& telemetry,
                                      std::int64_t arrival_ns) {
            return writer.append(telemetry, arrival_ns);
        };
        if (!append(event("phone", 10, 990'000'000, true), 1'000'000'000) ||
            !append(event("phone", 11, 1'005'000'000, false),
                    1'010'000'000) ||
            !append(event("phone", 13, 1'020'000'000, true),
                    1'030'000'000) ||
            !append(event("phone", 14, 1'000'000'000, false),
                    1'050'000'000) ||
            !append(fault(0, 1'055'000'000,
                          traceforge::v1::FaultEvent::SEVERITY_WARNING,
                          traceforge::v1::CLOCK_DOMAIN_UTC),
                    1'060'000'000) ||
            !append(fault(2, 123,
                          traceforge::v1::FaultEvent::SEVERITY_CRITICAL,
                          traceforge::v1::CLOCK_DOMAIN_DEVICE_MONOTONIC),
                    1'070'000'000)) {
            std::cerr << "Failed to write analysis fixture: " << writer.error()
                      << '\n';
            return 1;
        }
    }

    const auto report = traceforge::analysis::analyze_log(log.path());
    if (report.status != traceforge::analysis::AnalysisStatus::complete ||
        report.records != 6 || !report.duration_seconds.has_value() ||
        !near(*report.duration_seconds, 0.070) || report.producers.size() != 2 ||
        report.streams.size() != 3) {
        std::cerr << "Analysis summary is incorrect: " << report.message
                  << '\n';
        return 1;
    }

    const auto& fault_producer = report.producers[0];
    const auto& phone_producer = report.producers[1];
    if (fault_producer.producer_id != "fault-monitor" ||
        fault_producer.records != 2 || fault_producer.sequence_gaps != 1 ||
        phone_producer.producer_id != "phone" || phone_producer.records != 4 ||
        phone_producer.sequence_gaps != 1 ||
        phone_producer.non_increasing_sequences != 0) {
        std::cerr << "Producer sequence metrics are incorrect\n";
        return 1;
    }

    const auto& accelerometer = report.streams[1];
    const auto& gyroscope = report.streams[2];
    if (accelerometer.payload != "accelerometer" ||
        !accelerometer.observed_rate_hz.has_value() ||
        !accelerometer.interarrival_p50_ms.has_value() ||
        !accelerometer.jitter_p95_ms.has_value() ||
        !near(*accelerometer.observed_rate_hz, 33.333, 0.01) ||
        !near(*accelerometer.interarrival_p50_ms, 30.0) ||
        !near(*accelerometer.jitter_p95_ms, 0.0) ||
        gyroscope.payload != "gyroscope" ||
        gyroscope.source_timestamp_regressions != 1 ||
        !gyroscope.observed_rate_hz.has_value() ||
        !near(*gyroscope.observed_rate_hz, 25.0)) {
        std::cerr << "Per-stream timing metrics are incorrect\n";
        return 1;
    }

    if (!report.utc_arrival_delta.has_value() ||
        report.utc_arrival_delta->samples != 5 ||
        !near(report.utc_arrival_delta->p50_ms, 10.0) ||
        !near(report.utc_arrival_delta->p95_ms, 50.0) ||
        report.faults.warnings != 1 || report.faults.errors != 0 ||
        report.faults.critical != 1) {
        std::cerr << "Clock-delta or fault metrics are incorrect\n";
        return 1;
    }

    TemporaryLog empty{"empty"};
    { traceforge::recording::LogWriter writer{empty.path()}; }
    const auto empty_report = traceforge::analysis::analyze_log(empty.path());
    if (empty_report.status != traceforge::analysis::AnalysisStatus::complete ||
        empty_report.records != 0 || empty_report.duration_seconds.has_value() ||
        empty_report.utc_arrival_delta.has_value()) {
        std::cerr << "Empty log analysis is incorrect\n";
        return 1;
    }

    TemporaryLog truncated{"truncated"};
    std::filesystem::copy_file(log.path(), truncated.path());
    std::filesystem::resize_file(
        truncated.path(), std::filesystem::file_size(truncated.path()) - 1);
    const auto bad_report = traceforge::analysis::analyze_log(truncated.path());
    if (bad_report.status != traceforge::analysis::AnalysisStatus::log_error) {
        std::cerr << "Truncated log was accepted for analysis\n";
        return 1;
    }

    return 0;
}
