#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "traceforge/generator/workload_generator.hpp"
#include "traceforge/recording/log.hpp"
#include "traceforge/recording/log_consumer.hpp"
#include "traceforge/telemetry/queued_sink.hpp"

namespace {

class TemporaryLog {
  public:
    explicit TemporaryLog(std::string_view label) {
        static std::atomic<std::uint64_t> next_id{0};
        const auto id = next_id.fetch_add(1, std::memory_order_relaxed);
        const auto timestamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ =
            std::filesystem::temp_directory_path() /
            ("traceforge_" + std::string{label} + "_" +
             std::to_string(timestamp) + "_" + std::to_string(id) + ".tflog");
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

bool flip_byte(const std::filesystem::path& path, std::uint64_t offset) {
    std::fstream file{path, std::ios::binary | std::ios::in | std::ios::out};
    if (!file) {
        return false;
    }
    file.seekg(static_cast<std::streamoff>(offset));
    char value = 0;
    file.read(&value, 1);
    if (!file) {
        return false;
    }
    value = static_cast<char>(static_cast<unsigned char>(value) ^ 0x01U);
    file.seekp(static_cast<std::streamoff>(offset));
    file.write(&value, 1);
    file.flush();
    return file.good();
}

bool declare_oversized_first_payload(const std::filesystem::path& path) {
    std::fstream file{path, std::ios::binary | std::ios::in | std::ios::out};
    if (!file) {
        return false;
    }
    std::array<std::uint8_t, traceforge::recording::kRecordHeaderSize> header{};
    file.seekg(
        static_cast<std::streamoff>(traceforge::recording::kFileHeaderSize));
    file.read(reinterpret_cast<char*>(header.data()),
              static_cast<std::streamsize>(header.size()));
    if (!file) {
        return false;
    }

    const auto oversized = traceforge::recording::kMaximumPayloadSize + 1;
    for (std::size_t byte = 0; byte < sizeof(oversized); ++byte) {
        header[8 + byte] =
            static_cast<std::uint8_t>((oversized >> (byte * 8U)) & 0xFFU);
    }
    const auto header_crc =
        traceforge::recording::crc32c(std::span{header}.first(32));
    for (std::size_t byte = 0; byte < sizeof(header_crc); ++byte) {
        header[32 + byte] =
            static_cast<std::uint8_t>((header_crc >> (byte * 8U)) & 0xFFU);
    }

    file.seekp(
        static_cast<std::streamoff>(traceforge::recording::kFileHeaderSize));
    file.write(reinterpret_cast<const char*>(header.data()),
               static_cast<std::streamsize>(header.size()));
    file.flush();
    return file.good();
}

}  // namespace

int main() {
    const std::array<std::uint8_t, 9> crc_input{
        '1', '2', '3', '4', '5', '6', '7', '8', '9',
    };
    if (traceforge::recording::crc32c(crc_input) != 0xE3069283U) {
        std::cerr << "CRC32C implementation failed its standard check value\n";
        return 1;
    }

    traceforge::generator::GeneratorConfig generator_config;
    generator_config.seed = 42;
    generator_config.duration = std::chrono::milliseconds{100};
    const auto workload =
        traceforge::generator::WorkloadGenerator{generator_config}.generate();
    if (workload.events.empty()) {
        std::cerr << "Test workload is unexpectedly empty\n";
        return 1;
    }

    constexpr std::int64_t kCreatedNs = 1'777'000'000'000'000'000;
    TemporaryLog complete_log{"complete"};
    {
        traceforge::recording::LogWriter writer{
            complete_log.path(),
            {.flush_every_records = 3, .created_unix_ns = kCreatedNs},
        };
        if (!writer.good()) {
            std::cerr << writer.error() << '\n';
            return 1;
        }
        for (std::size_t index = 0; index < workload.events.size(); ++index) {
            if (!writer.append(workload.events[index],
                               kCreatedNs + static_cast<std::int64_t>(index))) {
                std::cerr << writer.error() << '\n';
                return 1;
            }
        }
        if (!writer.flush() ||
            writer.record_count() != workload.events.size() ||
            writer.bytes_written() <= traceforge::recording::kFileHeaderSize) {
            std::cerr << "Writer counters or flush result are incorrect\n";
            return 1;
        }
    }

    const auto complete = traceforge::recording::read_log(complete_log.path());
    if (complete.status != traceforge::recording::LogReadStatus::complete ||
        complete.created_unix_ns != kCreatedNs ||
        complete.records.size() != workload.events.size() ||
        complete.valid_bytes != complete.file_size) {
        std::cerr << "Complete log did not round trip: " << complete.message
                  << '\n';
        return 1;
    }
    traceforge::recording::LogWriter refusing_writer{complete_log.path()};
    if (refusing_writer.good() ||
        refusing_writer.error().find("refusing to overwrite") ==
            std::string::npos) {
        std::cerr << "Writer did not protect an existing recording\n";
        return 1;
    }
    for (std::size_t index = 0; index < complete.records.size(); ++index) {
        if (complete.records[index].record_index != index ||
            complete.records[index].collector_arrival_timestamp_ns !=
                kCreatedNs + static_cast<std::int64_t>(index) ||
            complete.records[index].event.SerializeAsString() !=
                workload.events[index].SerializeAsString()) {
            std::cerr << "A persisted record changed during round trip\n";
            return 1;
        }
    }

    TemporaryLog queued_log{"queued"};
    traceforge::recording::LogWriterConsumer recording_consumer{
        queued_log.path(),
        {.flush_every_records = 4, .created_unix_ns = kCreatedNs},
    };
    traceforge::telemetry::QueuedTelemetrySink recording_sink{
        workload.events.size(), recording_consumer};
    for (std::size_t index = 0; index < workload.events.size(); ++index) {
        traceforge::telemetry::CollectedEvent collected{
            .event = workload.events[index],
            .collector_arrival_timestamp_ns =
                kCreatedNs + static_cast<std::int64_t>(index),
        };
        if (recording_sink.try_accept(std::move(collected)) !=
            traceforge::telemetry::SinkResult::accepted) {
            std::cerr << "Recording queue unexpectedly rejected an event\n";
            return 1;
        }
    }
    recording_sink.shutdown();
    if (!recording_consumer.flush() ||
        recording_consumer.record_count() != workload.events.size()) {
        std::cerr << "Queue-backed recording consumer failed\n";
        return 1;
    }
    const auto queued_result =
        traceforge::recording::read_log(queued_log.path());
    if (queued_result.status !=
            traceforge::recording::LogReadStatus::complete ||
        queued_result.records.size() != workload.events.size()) {
        std::cerr << "Queue-backed recording did not produce a valid log\n";
        return 1;
    }

    TemporaryLog truncated_log{"truncated"};
    std::filesystem::copy_file(complete_log.path(), truncated_log.path());
    const auto original_size = std::filesystem::file_size(truncated_log.path());
    std::filesystem::resize_file(truncated_log.path(), original_size - 5);
    const auto truncated =
        traceforge::recording::read_log(truncated_log.path());
    if (truncated.status !=
            traceforge::recording::LogReadStatus::truncated_tail ||
        truncated.records.size() + 1 != workload.events.size() ||
        truncated.valid_bytes >= truncated.file_size) {
        std::cerr << "Incomplete final payload was not detected\n";
        return 1;
    }

    const auto recovery =
        traceforge::recording::recover_truncated_tail(truncated_log.path());
    if (!recovery.succeeded || !recovery.changed ||
        recovery.original_size != original_size - 5 ||
        recovery.recovered_size != truncated.valid_bytes) {
        std::cerr << "Truncated-tail recovery failed: " << recovery.message
                  << '\n';
        return 1;
    }
    const auto recovered =
        traceforge::recording::read_log(truncated_log.path());
    if (recovered.status != traceforge::recording::LogReadStatus::complete ||
        recovered.records.size() + 1 != workload.events.size()) {
        std::cerr << "Recovered file is not a complete valid log\n";
        return 1;
    }

    TemporaryLog corrupt_log{"corrupt"};
    std::filesystem::copy_file(complete_log.path(), corrupt_log.path());
    const auto payload_offset = traceforge::recording::kFileHeaderSize +
                                traceforge::recording::kRecordHeaderSize + 2;
    if (!flip_byte(corrupt_log.path(), payload_offset)) {
        std::cerr << "Failed to prepare corruption fixture\n";
        return 1;
    }
    const auto corrupt = traceforge::recording::read_log(corrupt_log.path());
    if (corrupt.status != traceforge::recording::LogReadStatus::corrupt ||
        corrupt.message.find("payload checksum") == std::string::npos) {
        std::cerr << "Payload corruption was not detected\n";
        return 1;
    }
    const auto refused_recovery =
        traceforge::recording::recover_truncated_tail(corrupt_log.path());
    if (refused_recovery.succeeded || refused_recovery.changed) {
        std::cerr << "Recovery must refuse completed-record corruption\n";
        return 1;
    }

    TemporaryLog bad_header_log{"bad_header"};
    std::filesystem::copy_file(complete_log.path(), bad_header_log.path());
    if (!flip_byte(bad_header_log.path(), 0) ||
        traceforge::recording::read_log(bad_header_log.path()).status !=
            traceforge::recording::LogReadStatus::corrupt) {
        std::cerr << "File-header corruption was not detected\n";
        return 1;
    }

    TemporaryLog oversized_log{"oversized"};
    std::filesystem::copy_file(complete_log.path(), oversized_log.path());
    if (!declare_oversized_first_payload(oversized_log.path())) {
        std::cerr << "Failed to prepare oversized-length fixture\n";
        return 1;
    }
    const auto oversized =
        traceforge::recording::read_log(oversized_log.path());
    if (oversized.status !=
            traceforge::recording::LogReadStatus::resource_limit ||
        oversized.message.find("format limit") == std::string::npos) {
        std::cerr << "Oversized declared payload was not rejected safely\n";
        return 1;
    }

    return 0;
}
