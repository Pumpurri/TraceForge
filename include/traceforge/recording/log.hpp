#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include "traceforge/v1/telemetry.pb.h"

namespace traceforge::recording {

inline constexpr std::uint16_t kLogFormatVersion = 1;
inline constexpr std::uint16_t kRecordFormatVersion = 1;
inline constexpr std::size_t kFileHeaderSize = 32;
inline constexpr std::size_t kRecordHeaderSize = 36;
inline constexpr std::uint32_t kMaximumPayloadSize = 4 * 1'024 * 1'024;

[[nodiscard]] std::uint32_t
crc32c(std::span<const std::uint8_t> bytes) noexcept;

struct LogWriterOptions {
    std::uint64_t flush_every_records{64};
    std::int64_t created_unix_ns{0};
};

class LogWriter {
  public:
    explicit LogWriter(std::filesystem::path path,
                       LogWriterOptions options = {});
    ~LogWriter();

    LogWriter(const LogWriter&) = delete;
    LogWriter& operator=(const LogWriter&) = delete;

    [[nodiscard]] bool append(const v1::TelemetryEvent& event,
                              std::int64_t collector_arrival_timestamp_ns);
    [[nodiscard]] bool flush();
    [[nodiscard]] bool good() const noexcept;
    [[nodiscard]] std::string error() const;
    [[nodiscard]] std::uint64_t record_count() const noexcept;
    [[nodiscard]] std::uint64_t bytes_written() const noexcept;

  private:
    bool write_file_header();
    bool write_bytes(std::span<const std::uint8_t> bytes);
    void fail(std::string message);

    std::filesystem::path path_;
    LogWriterOptions options_;
    std::ofstream output_;
    std::string error_;
    std::uint64_t record_count_{0};
    std::uint64_t bytes_written_{0};
};

enum class LogReadStatus {
    complete,
    truncated_tail,
    corrupt,
    io_error,
    resource_limit,
};

struct LogRecord {
    std::uint64_t record_index{0};
    std::uint64_t file_offset{0};
    std::int64_t collector_arrival_timestamp_ns{0};
    v1::TelemetryEvent event;
};

struct LogIndexEntry {
    std::uint64_t record_index{0};
    std::uint64_t file_offset{0};
    std::uint32_t payload_size{0};
    std::uint32_t payload_crc32c{0};
    std::int64_t collector_arrival_timestamp_ns{0};
    std::int64_t source_timestamp_ns{0};
};

struct LogReadOptions {
    std::size_t maximum_records{1'000'000};
    bool retain_records{true};
};

struct LogReadResult {
    LogReadStatus status{LogReadStatus::io_error};
    std::int64_t created_unix_ns{0};
    std::uint64_t file_size{0};
    std::uint64_t valid_bytes{0};
    std::uint64_t error_offset{0};
    std::string message;
    std::vector<LogIndexEntry> index;
    std::vector<LogRecord> records;
};

[[nodiscard]] LogReadResult read_log(const std::filesystem::path& path,
                                     LogReadOptions options = {});
[[nodiscard]] std::string_view status_name(LogReadStatus status) noexcept;

struct RecoveryResult {
    bool succeeded{false};
    bool changed{false};
    std::uint64_t original_size{0};
    std::uint64_t recovered_size{0};
    std::string message;
};

[[nodiscard]] RecoveryResult
recover_truncated_tail(const std::filesystem::path& path,
                       LogReadOptions options = {});

}  // namespace traceforge::recording
