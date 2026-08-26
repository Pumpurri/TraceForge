#include "traceforge/recording/log.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <ios>
#include <limits>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "traceforge/telemetry/validation.hpp"

namespace traceforge::recording {
namespace {

constexpr std::array<std::uint8_t, 8> kFileMagic{
    'T', 'F', 'L', 'O', 'G', '\r', '\n', 0x1A,
};
constexpr std::array<std::uint8_t, 4> kRecordMagic{'T', 'F', 'R', '1'};

template <typename Integer>
void encode_little_endian(std::span<std::uint8_t> destination,
                          std::size_t offset, Integer value) {
    using Unsigned = std::make_unsigned_t<Integer>;
    auto remaining = static_cast<Unsigned>(value);
    for (std::size_t byte = 0; byte < sizeof(Integer); ++byte) {
        destination[offset + byte] =
            static_cast<std::uint8_t>(remaining & static_cast<Unsigned>(0xFF));
        remaining >>= 8U;
    }
}

template <typename Integer>
[[nodiscard]] Integer decode_little_endian(std::span<const std::uint8_t> source,
                                           std::size_t offset) {
    using Unsigned = std::make_unsigned_t<Integer>;
    Unsigned value = 0;
    for (std::size_t byte = 0; byte < sizeof(Integer); ++byte) {
        value |= static_cast<Unsigned>(source[offset + byte]) << (byte * 8U);
    }
    return static_cast<Integer>(value);
}

[[nodiscard]] std::int64_t unix_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

[[nodiscard]] bool read_exact(std::ifstream& input,
                              std::span<std::uint8_t> destination) {
    input.read(reinterpret_cast<char*>(destination.data()),
               static_cast<std::streamsize>(destination.size()));
    return input.gcount() == static_cast<std::streamsize>(destination.size());
}

[[nodiscard]] LogReadResult make_failure(LogReadResult result,
                                         LogReadStatus status,
                                         std::uint64_t offset,
                                         std::string message) {
    result.status = status;
    result.error_offset = offset;
    result.message = std::move(message);
    return result;
}

}  // namespace

std::uint32_t crc32c(std::span<const std::uint8_t> bytes) noexcept {
    constexpr std::uint32_t kReflectedCastagnoliPolynomial = 0x82F63B78U;
    std::uint32_t checksum = 0xFFFFFFFFU;
    for (const auto byte : bytes) {
        checksum ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0U - (checksum & 1U);
            checksum =
                (checksum >> 1U) ^ (kReflectedCastagnoliPolynomial & mask);
        }
    }
    return ~checksum;
}

LogWriter::LogWriter(std::filesystem::path path, LogWriterOptions options)
    : path_(std::move(path)), options_(options) {
    if (options_.flush_every_records == 0) {
        fail("flush_every_records must be positive");
        return;
    }
    if (options_.created_unix_ns == 0) {
        options_.created_unix_ns = unix_time_ns();
    }

    std::error_code exists_error;
    const bool already_exists = std::filesystem::exists(path_, exists_error);
    if (exists_error) {
        fail("failed to check whether output log already exists");
        return;
    }
    if (already_exists) {
        fail("refusing to overwrite existing telemetry log: " + path_.string());
        return;
    }

    output_.open(path_, std::ios::binary | std::ios::trunc);
    if (!output_) {
        fail("failed to open log for writing: " + path_.string());
        return;
    }
    static_cast<void>(write_file_header());
}

LogWriter::~LogWriter() {
    if (output_.is_open()) {
        output_.flush();
    }
}

bool LogWriter::append(const v1::TelemetryEvent& event,
                       std::int64_t collector_arrival_timestamp_ns) {
    if (!good()) {
        return false;
    }
    if (event.schema_version() != telemetry::kTelemetrySchemaVersion) {
        fail("refusing to write an unsupported telemetry schema version");
        return false;
    }

    std::string payload;
    if (!event.SerializeToString(&payload)) {
        fail("failed to serialize telemetry event");
        return false;
    }
    if (payload.size() > kMaximumPayloadSize) {
        fail("telemetry payload exceeds the format limit");
        return false;
    }

    const auto payload_bytes = std::span{
        reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()};
    std::array<std::uint8_t, kRecordHeaderSize> header{};
    std::copy(kRecordMagic.begin(), kRecordMagic.end(), header.begin());
    encode_little_endian<std::uint16_t>(header, 4, kRecordHeaderSize);
    encode_little_endian<std::uint16_t>(header, 6, kRecordFormatVersion);
    encode_little_endian<std::uint32_t>(
        header, 8, static_cast<std::uint32_t>(payload.size()));
    encode_little_endian<std::uint32_t>(header, 12, crc32c(payload_bytes));
    encode_little_endian<std::uint64_t>(header, 16, record_count_);
    encode_little_endian<std::int64_t>(header, 24,
                                       collector_arrival_timestamp_ns);
    encode_little_endian<std::uint32_t>(header, 32,
                                        crc32c(std::span{header}.first(32)));

    if (!write_bytes(header) || !write_bytes(payload_bytes)) {
        return false;
    }

    ++record_count_;
    if (record_count_ % options_.flush_every_records == 0 && !flush()) {
        return false;
    }
    return true;
}

bool LogWriter::flush() {
    if (!good()) {
        return false;
    }
    output_.flush();
    if (!output_) {
        fail("failed to flush telemetry log");
        return false;
    }
    return true;
}

bool LogWriter::good() const noexcept {
    return error_.empty() && output_.is_open() && output_.good();
}

std::string LogWriter::error() const { return error_; }

std::uint64_t LogWriter::record_count() const noexcept { return record_count_; }

std::uint64_t LogWriter::bytes_written() const noexcept {
    return bytes_written_;
}

bool LogWriter::write_file_header() {
    std::array<std::uint8_t, kFileHeaderSize> header{};
    std::copy(kFileMagic.begin(), kFileMagic.end(), header.begin());
    encode_little_endian<std::uint16_t>(header, 8, kLogFormatVersion);
    encode_little_endian<std::uint16_t>(header, 10, kFileHeaderSize);
    encode_little_endian<std::uint32_t>(header, 12, 0);
    encode_little_endian<std::int64_t>(header, 16, options_.created_unix_ns);
    encode_little_endian<std::uint32_t>(header, 24, 0);
    encode_little_endian<std::uint32_t>(header, 28,
                                        crc32c(std::span{header}.first(28)));
    return write_bytes(header);
}

bool LogWriter::write_bytes(std::span<const std::uint8_t> bytes) {
    output_.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    if (!output_) {
        fail("failed while writing telemetry log");
        return false;
    }
    bytes_written_ += bytes.size();
    return true;
}

void LogWriter::fail(std::string message) {
    if (error_.empty()) {
        error_ = std::move(message);
    }
}

LogReadResult read_log(const std::filesystem::path& path,
                       LogReadOptions options) {
    LogReadResult result;
    std::error_code filesystem_error;
    result.file_size = std::filesystem::file_size(path, filesystem_error);
    if (filesystem_error) {
        return make_failure(std::move(result), LogReadStatus::io_error, 0,
                            "failed to determine log file size");
    }

    std::ifstream input{path, std::ios::binary};
    if (!input) {
        return make_failure(std::move(result), LogReadStatus::io_error, 0,
                            "failed to open log file");
    }
    if (result.file_size < kFileHeaderSize) {
        return make_failure(std::move(result), LogReadStatus::corrupt, 0,
                            "file header is incomplete");
    }

    std::array<std::uint8_t, kFileHeaderSize> file_header{};
    if (!read_exact(input, file_header)) {
        return make_failure(std::move(result), LogReadStatus::io_error, 0,
                            "failed to read file header");
    }
    if (!std::equal(kFileMagic.begin(), kFileMagic.end(),
                    file_header.begin())) {
        return make_failure(std::move(result), LogReadStatus::corrupt, 0,
                            "invalid log magic");
    }
    if (decode_little_endian<std::uint16_t>(file_header, 8) !=
            kLogFormatVersion ||
        decode_little_endian<std::uint16_t>(file_header, 10) !=
            kFileHeaderSize) {
        return make_failure(std::move(result), LogReadStatus::corrupt, 0,
                            "unsupported log header version or size");
    }
    if (decode_little_endian<std::uint32_t>(file_header, 28) !=
        crc32c(std::span{file_header}.first(28))) {
        return make_failure(std::move(result), LogReadStatus::corrupt, 0,
                            "file header checksum mismatch");
    }

    result.created_unix_ns =
        decode_little_endian<std::int64_t>(file_header, 16);
    result.valid_bytes = kFileHeaderSize;
    std::uint64_t offset = kFileHeaderSize;
    std::uint64_t expected_record_index = 0;
    while (offset < result.file_size) {
        if (result.index.size() == options.maximum_records) {
            return make_failure(std::move(result),
                                LogReadStatus::resource_limit, offset,
                                "record-count limit reached");
        }
        if (result.file_size - offset < kRecordHeaderSize) {
            return make_failure(std::move(result),
                                LogReadStatus::truncated_tail, offset,
                                "final record header is incomplete");
        }

        std::array<std::uint8_t, kRecordHeaderSize> record_header{};
        if (!read_exact(input, record_header)) {
            return make_failure(std::move(result), LogReadStatus::io_error,
                                offset, "failed to read record header");
        }
        if (!std::equal(kRecordMagic.begin(), kRecordMagic.end(),
                        record_header.begin())) {
            return make_failure(std::move(result), LogReadStatus::corrupt,
                                offset, "invalid record magic");
        }
        if (decode_little_endian<std::uint16_t>(record_header, 4) !=
                kRecordHeaderSize ||
            decode_little_endian<std::uint16_t>(record_header, 6) !=
                kRecordFormatVersion) {
            return make_failure(std::move(result), LogReadStatus::corrupt,
                                offset,
                                "unsupported record header version or size");
        }
        if (decode_little_endian<std::uint32_t>(record_header, 32) !=
            crc32c(std::span{record_header}.first(32))) {
            return make_failure(std::move(result), LogReadStatus::corrupt,
                                offset, "record header checksum mismatch");
        }

        const auto payload_size =
            decode_little_endian<std::uint32_t>(record_header, 8);
        if (payload_size > kMaximumPayloadSize) {
            return make_failure(std::move(result),
                                LogReadStatus::resource_limit, offset,
                                "record payload exceeds format limit");
        }
        if (result.file_size - offset - kRecordHeaderSize < payload_size) {
            return make_failure(std::move(result),
                                LogReadStatus::truncated_tail, offset,
                                "final record payload is incomplete");
        }

        std::vector<std::uint8_t> payload(payload_size);
        if (!read_exact(input, payload)) {
            return make_failure(std::move(result), LogReadStatus::io_error,
                                offset, "failed to read record payload");
        }
        if (decode_little_endian<std::uint32_t>(record_header, 12) !=
            crc32c(payload)) {
            return make_failure(std::move(result), LogReadStatus::corrupt,
                                offset, "record payload checksum mismatch");
        }

        LogRecord record;
        record.record_index =
            decode_little_endian<std::uint64_t>(record_header, 16);
        if (record.record_index != expected_record_index) {
            return make_failure(std::move(result), LogReadStatus::corrupt,
                                offset, "record index is not contiguous");
        }
        record.file_offset = offset;
        record.collector_arrival_timestamp_ns =
            decode_little_endian<std::int64_t>(record_header, 24);
        if (!record.event.ParseFromArray(payload.data(),
                                         static_cast<int>(payload.size()))) {
            return make_failure(std::move(result), LogReadStatus::corrupt,
                                offset,
                                "telemetry payload is not valid Protobuf");
        }
        if (record.event.schema_version() !=
            telemetry::kTelemetrySchemaVersion) {
            return make_failure(std::move(result), LogReadStatus::corrupt,
                                offset, "unsupported telemetry schema version");
        }

        result.index.push_back(LogIndexEntry{
            .record_index = record.record_index,
            .file_offset = offset,
            .payload_size = payload_size,
            .payload_crc32c =
                decode_little_endian<std::uint32_t>(record_header, 12),
            .collector_arrival_timestamp_ns =
                record.collector_arrival_timestamp_ns,
            .source_timestamp_ns = record.event.source_timestamp_ns(),
        });
        if (options.retain_records) {
            result.records.push_back(std::move(record));
        }
        ++expected_record_index;
        offset += kRecordHeaderSize + payload_size;
        result.valid_bytes = offset;
    }

    result.status = LogReadStatus::complete;
    result.message = "log is complete";
    return result;
}

std::string_view status_name(LogReadStatus status) noexcept {
    switch (status) {
    case LogReadStatus::complete:
        return "complete";
    case LogReadStatus::truncated_tail:
        return "truncated_tail";
    case LogReadStatus::corrupt:
        return "corrupt";
    case LogReadStatus::io_error:
        return "io_error";
    case LogReadStatus::resource_limit:
        return "resource_limit";
    }
    return "unknown";
}

RecoveryResult recover_truncated_tail(const std::filesystem::path& path,
                                      LogReadOptions options) {
    const auto read_result = read_log(path, options);
    RecoveryResult recovery{
        .original_size = read_result.file_size,
        .recovered_size = read_result.file_size,
    };
    if (read_result.status == LogReadStatus::complete) {
        recovery.succeeded = true;
        recovery.message = "log was already complete";
        return recovery;
    }
    if (read_result.status != LogReadStatus::truncated_tail ||
        read_result.valid_bytes < kFileHeaderSize) {
        recovery.message = "refusing recovery because the log is not a "
                           "recoverable truncated tail: " +
                           read_result.message;
        return recovery;
    }

    std::error_code error;
    std::filesystem::resize_file(path, read_result.valid_bytes, error);
    if (error) {
        recovery.message = "failed to truncate incomplete tail";
        return recovery;
    }
    recovery.succeeded = true;
    recovery.changed = true;
    recovery.recovered_size = read_result.valid_bytes;
    recovery.message = "removed incomplete final record";
    return recovery;
}

}  // namespace traceforge::recording
