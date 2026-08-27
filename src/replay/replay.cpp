#include "traceforge/replay/replay.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <sstream>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "traceforge/telemetry/validation.hpp"

namespace traceforge::replay {
namespace {

constexpr std::uint64_t kFnvOffsetBasis = 14'695'981'039'346'656'037ULL;
constexpr std::uint64_t kFnvPrime = 1'099'511'628'211ULL;

void hash_byte(std::uint64_t& hash, std::uint8_t byte) noexcept {
    hash ^= byte;
    hash *= kFnvPrime;
}

template <typename Integer>
void hash_little_endian(std::uint64_t& hash, Integer value) noexcept {
    using Unsigned = std::make_unsigned_t<Integer>;
    auto remaining = static_cast<Unsigned>(value);
    for (std::size_t byte = 0; byte < sizeof(Integer); ++byte) {
        hash_byte(hash, static_cast<std::uint8_t>(remaining & 0xFFU));
        remaining >>= 8U;
    }
}

[[nodiscard]] bool read_payload(std::ifstream& input,
                                const recording::LogIndexEntry& entry,
                                bool sequential_record,
                                std::vector<std::uint8_t>& payload) {
    input.clear();
    if (sequential_record) {
        input.ignore(static_cast<std::streamsize>(
            recording::kRecordHeaderSize));
    } else {
        input.seekg(static_cast<std::streamoff>(
            entry.file_offset + recording::kRecordHeaderSize));
    }
    if (!input) {
        return false;
    }
    payload.resize(entry.payload_size);
    input.read(reinterpret_cast<char*>(payload.data()),
               static_cast<std::streamsize>(payload.size()));
    return input.gcount() == static_cast<std::streamsize>(payload.size());
}

}  // namespace

ReplayEngine::ReplayEngine(std::filesystem::path path,
                           std::size_t maximum_records)
    : path_(std::move(path)) {
    auto log = recording::read_log(
        path_, {.maximum_records = maximum_records, .retain_records = false});
    if (log.status != recording::LogReadStatus::complete) {
        error_ = "cannot index telemetry log: " + log.message;
        return;
    }

    index_ = std::move(log.index);
    std::sort(index_.begin(), index_.end(),
              [](const recording::LogIndexEntry& left,
                 const recording::LogIndexEntry& right) {
                  if (left.collector_arrival_timestamp_ns !=
                      right.collector_arrival_timestamp_ns) {
                      return left.collector_arrival_timestamp_ns <
                             right.collector_arrival_timestamp_ns;
                  }
                  return left.record_index < right.record_index;
              });
}

bool ReplayEngine::good() const noexcept { return error_.empty(); }

std::string ReplayEngine::error() const { return error_; }

std::span<const recording::LogIndexEntry> ReplayEngine::index() const noexcept {
    return index_;
}

ReplayResult ReplayEngine::replay(ReplayOptions options,
                                  ReplayConsumer consumer) const {
    ReplayResult result{
        .indexed_records = static_cast<std::uint64_t>(index_.size()),
        .hash = kFnvOffsetBasis,
    };
    if (!good()) {
        result.status = ReplayStatus::log_error;
        result.message = error_;
        return result;
    }
    if (!std::isfinite(options.speed) || options.speed < 0.0) {
        result.status = ReplayStatus::invalid_options;
        result.message = "replay speed must be finite and non-negative";
        return result;
    }
    if (options.from_collector_timestamp_ns.has_value() &&
        options.until_collector_timestamp_ns.has_value() &&
        *options.until_collector_timestamp_ns <=
            *options.from_collector_timestamp_ns) {
        result.status = ReplayStatus::invalid_options;
        result.message = "until timestamp must be greater than from timestamp";
        return result;
    }

    auto current = index_.begin();
    if (options.from_collector_timestamp_ns.has_value()) {
        current = std::lower_bound(
            index_.begin(), index_.end(),
            *options.from_collector_timestamp_ns,
            [](const recording::LogIndexEntry& entry, std::int64_t timestamp) {
                return entry.collector_arrival_timestamp_ns < timestamp;
            });
    }

    std::ifstream input{path_, std::ios::binary};
    if (!input) {
        result.status = ReplayStatus::io_error;
        result.message = "failed to open indexed telemetry log";
        return result;
    }

    std::optional<std::int64_t> replay_origin_ns;
    std::optional<std::chrono::steady_clock::time_point> wall_origin;
    std::optional<std::uint64_t> next_physical_offset;
    std::vector<std::uint8_t> payload;
    for (; current != index_.end(); ++current) {
        const auto timestamp = current->collector_arrival_timestamp_ns;
        if (options.until_collector_timestamp_ns.has_value() &&
            timestamp >= *options.until_collector_timestamp_ns) {
            break;
        }
        const bool sequential_record =
            next_physical_offset.has_value() &&
            *next_physical_offset == current->file_offset;
        if (!read_payload(input, *current, sequential_record, payload)) {
            result.status = ReplayStatus::io_error;
            result.message = "failed to read indexed record payload";
            return result;
        }
        next_physical_offset = current->file_offset +
                               recording::kRecordHeaderSize +
                               current->payload_size;
        if (recording::crc32c(payload) != current->payload_crc32c) {
            result.status = ReplayStatus::log_error;
            result.message = "record payload changed after index construction";
            return result;
        }

        recording::LogRecord record{
            .record_index = current->record_index,
            .file_offset = current->file_offset,
            .collector_arrival_timestamp_ns = timestamp,
        };
        if (!record.event.ParseFromArray(payload.data(),
                                         static_cast<int>(payload.size())) ||
            record.event.schema_version() !=
                telemetry::kTelemetrySchemaVersion ||
            record.event.source_timestamp_ns() !=
                current->source_timestamp_ns) {
            result.status = ReplayStatus::log_error;
            result.message = "indexed telemetry payload is no longer valid";
            return result;
        }

        if (options.speed > 0.0) {
            if (!replay_origin_ns.has_value()) {
                replay_origin_ns = timestamp;
                wall_origin = std::chrono::steady_clock::now();
            }
            const long double elapsed_ns =
                static_cast<long double>(timestamp) -
                static_cast<long double>(*replay_origin_ns);
            const auto scaled = std::chrono::duration<long double, std::nano>{
                elapsed_ns / static_cast<long double>(options.speed)};
            std::this_thread::sleep_until(
                *wall_origin +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    scaled));
        }

        hash_little_endian(result.hash, record.record_index);
        hash_little_endian(result.hash,
                           record.collector_arrival_timestamp_ns);
        hash_little_endian(result.hash,
                           static_cast<std::uint32_t>(payload.size()));
        for (const auto byte : payload) {
            hash_byte(result.hash, byte);
        }

        if (consumer && !consumer(record)) {
            result.status = ReplayStatus::consumer_error;
            result.message = "replay consumer rejected a record";
            return result;
        }
        if (!result.first_timestamp_ns.has_value()) {
            result.first_timestamp_ns = timestamp;
        }
        result.last_timestamp_ns = timestamp;
        ++result.replayed_records;
    }

    result.status = ReplayStatus::complete;
    result.message = "replay completed";
    return result;
}

std::string_view status_name(ReplayStatus status) noexcept {
    switch (status) {
    case ReplayStatus::complete:
        return "complete";
    case ReplayStatus::invalid_options:
        return "invalid_options";
    case ReplayStatus::log_error:
        return "log_error";
    case ReplayStatus::io_error:
        return "io_error";
    case ReplayStatus::consumer_error:
        return "consumer_error";
    }
    return "unknown";
}

std::string format_hash(std::uint64_t hash) {
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(16) << hash;
    return output.str();
}

}  // namespace traceforge::replay
