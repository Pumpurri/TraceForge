#include "traceforge/analysis/analyzer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "traceforge/generator/workload_generator.hpp"
#include "traceforge/recording/log.hpp"
#include "traceforge/v1/telemetry.pb.h"

namespace traceforge::analysis {
namespace {

constexpr long double kNanosecondsPerMillisecond = 1'000'000.0L;
constexpr long double kNanosecondsPerSecond = 1'000'000'000.0L;

using StreamKey = std::pair<std::string, std::string>;
using SourceClockKey = std::tuple<std::string, std::string, int>;

struct ProducerAccumulator {
    ProducerStats stats;
    std::optional<std::uint64_t> previous_sequence;
};

struct StreamAccumulator {
    StreamStats stats;
    std::vector<std::int64_t> collector_timestamps;
};

[[nodiscard]] double nanoseconds_to_milliseconds(long double nanoseconds) {
    return static_cast<double>(nanoseconds / kNanosecondsPerMillisecond);
}

[[nodiscard]] double percentile(std::vector<double> values, double fraction) {
    std::sort(values.begin(), values.end());
    const auto rank = static_cast<std::size_t>(
        std::ceil(fraction * static_cast<double>(values.size())));
    const auto index = rank == 0 ? std::size_t{0} : rank - 1;
    return values[std::min(index, values.size() - 1)];
}

[[nodiscard]] DistributionStats distribution(std::vector<double> values) {
    DistributionStats result;
    result.samples = static_cast<std::uint64_t>(values.size());
    result.p50_ms = percentile(values, 0.50);
    result.p95_ms = percentile(values, 0.95);
    result.p99_ms = percentile(values, 0.99);
    result.maximum_ms = *std::max_element(values.begin(), values.end());
    return result;
}

void finish_stream(StreamAccumulator& stream) {
    auto& timestamps = stream.collector_timestamps;
    if (timestamps.size() < 2) {
        return;
    }

    std::sort(timestamps.begin(), timestamps.end());
    const long double span_ns = static_cast<long double>(timestamps.back()) -
                                static_cast<long double>(timestamps.front());
    if (span_ns > 0.0L) {
        stream.stats.observed_rate_hz =
            static_cast<double>(
                static_cast<long double>(timestamps.size() - 1) *
                kNanosecondsPerSecond / span_ns);
    }

    std::vector<double> intervals_ms;
    intervals_ms.reserve(timestamps.size() - 1);
    for (std::size_t index = 1; index < timestamps.size(); ++index) {
        const long double interval_ns =
            static_cast<long double>(timestamps[index]) -
            static_cast<long double>(timestamps[index - 1]);
        intervals_ms.push_back(nanoseconds_to_milliseconds(interval_ns));
    }

    const double median_interval_ms = percentile(intervals_ms, 0.50);
    stream.stats.interarrival_p50_ms = median_interval_ms;

    std::vector<double> absolute_deviations_ms;
    absolute_deviations_ms.reserve(intervals_ms.size());
    for (const double interval_ms : intervals_ms) {
        absolute_deviations_ms.push_back(
            std::abs(interval_ms - median_interval_ms));
    }
    stream.stats.jitter_p95_ms =
        percentile(std::move(absolute_deviations_ms), 0.95);
}

}  // namespace

AnalysisReport analyze_log(const std::filesystem::path& path,
                           std::size_t maximum_records) {
    AnalysisReport report;
    auto log = recording::read_log(
        path, {.maximum_records = maximum_records, .retain_records = true});
    report.file_bytes = log.file_size;
    if (log.status != recording::LogReadStatus::complete) {
        report.message = "cannot analyze telemetry log: " + log.message;
        return report;
    }

    report.records = static_cast<std::uint64_t>(log.records.size());
    std::map<std::string, ProducerAccumulator> producers;
    std::map<StreamKey, StreamAccumulator> streams;
    std::map<SourceClockKey, std::int64_t> previous_source_timestamps;
    std::vector<double> utc_arrival_delta_ms;
    std::optional<std::int64_t> first_collector_timestamp;
    std::optional<std::int64_t> last_collector_timestamp;

    for (const auto& record : log.records) {
        const auto& event = record.event;
        const std::string payload{
            generator::payload_name(event.payload_case())};

        auto& producer = producers[event.producer_id()];
        producer.stats.producer_id = event.producer_id();
        ++producer.stats.records;
        if (producer.previous_sequence.has_value()) {
            if (event.sequence_number() <= *producer.previous_sequence) {
                ++producer.stats.non_increasing_sequences;
            } else {
                producer.stats.sequence_gaps +=
                    event.sequence_number() - *producer.previous_sequence - 1;
            }
        }
        producer.previous_sequence = event.sequence_number();

        const StreamKey stream_key{event.producer_id(), payload};
        auto& stream = streams[stream_key];
        stream.stats.producer_id = event.producer_id();
        stream.stats.payload = payload;
        ++stream.stats.records;
        stream.collector_timestamps.push_back(
            record.collector_arrival_timestamp_ns);

        const SourceClockKey source_key{
            event.producer_id(), payload, static_cast<int>(event.source_clock())};
        const auto previous_source = previous_source_timestamps.find(source_key);
        if (previous_source != previous_source_timestamps.end() &&
            event.source_timestamp_ns() <= previous_source->second) {
            ++stream.stats.source_timestamp_regressions;
        }
        previous_source_timestamps[source_key] = event.source_timestamp_ns();

        if (event.source_clock() == v1::CLOCK_DOMAIN_UTC) {
            const long double delta_ns =
                static_cast<long double>(
                    record.collector_arrival_timestamp_ns) -
                static_cast<long double>(event.source_timestamp_ns());
            utc_arrival_delta_ms.push_back(
                nanoseconds_to_milliseconds(delta_ns));
        }

        if (!first_collector_timestamp.has_value() ||
            record.collector_arrival_timestamp_ns <
                *first_collector_timestamp) {
            first_collector_timestamp =
                record.collector_arrival_timestamp_ns;
        }
        if (!last_collector_timestamp.has_value() ||
            record.collector_arrival_timestamp_ns > *last_collector_timestamp) {
            last_collector_timestamp = record.collector_arrival_timestamp_ns;
        }

        if (event.payload_case() == v1::TelemetryEvent::kFault) {
            switch (event.fault().severity()) {
            case v1::FaultEvent::SEVERITY_WARNING:
                ++report.faults.warnings;
                break;
            case v1::FaultEvent::SEVERITY_ERROR:
                ++report.faults.errors;
                break;
            case v1::FaultEvent::SEVERITY_CRITICAL:
                ++report.faults.critical;
                break;
            case v1::FaultEvent::SEVERITY_UNSPECIFIED:
            default:
                break;
            }
        }
    }

    if (first_collector_timestamp.has_value()) {
        const long double duration_ns =
            static_cast<long double>(*last_collector_timestamp) -
            static_cast<long double>(*first_collector_timestamp);
        report.duration_seconds =
            static_cast<double>(duration_ns / kNanosecondsPerSecond);
    }

    report.producers.reserve(producers.size());
    for (auto& [producer_id, producer] : producers) {
        static_cast<void>(producer_id);
        report.producers.push_back(std::move(producer.stats));
    }

    report.streams.reserve(streams.size());
    for (auto& [stream_key, stream] : streams) {
        static_cast<void>(stream_key);
        finish_stream(stream);
        report.streams.push_back(std::move(stream.stats));
    }

    if (!utc_arrival_delta_ms.empty()) {
        report.utc_arrival_delta =
            distribution(std::move(utc_arrival_delta_ms));
    }
    report.status = AnalysisStatus::complete;
    report.message = "analysis completed";
    return report;
}

std::string_view status_name(AnalysisStatus status) noexcept {
    switch (status) {
    case AnalysisStatus::complete:
        return "complete";
    case AnalysisStatus::log_error:
        return "log_error";
    }
    return "unknown";
}

}  // namespace traceforge::analysis
