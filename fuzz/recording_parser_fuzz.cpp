#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>

#include "traceforge/recording/log.hpp"
#include "traceforge/telemetry/validation.hpp"

namespace {

class ScratchLog {
  public:
    ScratchLog() {
        const auto timestamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("traceforge_recording_fuzz_" + std::to_string(timestamp) +
                 ".tflog");
    }

    ~ScratchLog() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

void exercise_parser(const std::filesystem::path& path) {
    const auto result = traceforge::recording::read_log(
        path, {.maximum_records = 4'096, .retain_records = false});
    if (result.status ==
        traceforge::recording::LogReadStatus::truncated_tail) {
        const auto recovery = traceforge::recording::recover_truncated_tail(
            path, {.maximum_records = 4'096, .retain_records = false});
        if (recovery.succeeded) {
            static_cast<void>(traceforge::recording::read_log(
                path, {.maximum_records = 4'096, .retain_records = false}));
        }
    }
}

bool write_raw(const std::filesystem::path& path, const std::uint8_t* data,
               std::size_t size) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    if (!output) {
        return false;
    }
    output.write(reinterpret_cast<const char*>(data),
                 static_cast<std::streamsize>(size));
    output.close();
    return output.good();
}

bool write_mutated_valid_log(const std::filesystem::path& path,
                             const std::uint8_t* mutations,
                             std::size_t mutation_size) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    {
        traceforge::recording::LogWriter writer{
            path, {.flush_every_records = 1, .created_unix_ns = 1}};
        traceforge::v1::TelemetryEvent event;
        event.set_schema_version(
            traceforge::telemetry::kTelemetrySchemaVersion);
        event.set_producer_id("fuzz-seed");
        event.set_sequence_number(0);
        event.set_source_timestamp_ns(1);
        event.set_source_clock(traceforge::v1::CLOCK_DOMAIN_UTC);
        event.mutable_temperature()->set_degrees_celsius(42.0);
        if (!writer.append(event, 2) || !writer.flush()) {
            return false;
        }
    }

    const auto file_size = std::filesystem::file_size(path, ignored);
    if (ignored || file_size == 0) {
        return false;
    }
    std::fstream file{path, std::ios::binary | std::ios::in | std::ios::out};
    if (!file) {
        return false;
    }
    for (std::size_t index = 0; index < mutation_size; index += 2) {
        const auto offset = static_cast<std::uint64_t>(mutations[index]) %
                            static_cast<std::uint64_t>(file_size);
        const auto mask = index + 1 < mutation_size ? mutations[index + 1]
                                                     : std::uint8_t{0xFF};
        file.seekg(static_cast<std::streamoff>(offset));
        char value = 0;
        file.read(&value, 1);
        if (!file) {
            return false;
        }
        value = static_cast<char>(static_cast<unsigned char>(value) ^ mask);
        file.seekp(static_cast<std::streamoff>(offset));
        file.write(&value, 1);
        if (!file) {
            return false;
        }
    }
    file.flush();
    return file.good();
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    constexpr std::size_t kMaximumFuzzInput = 8 * 1'024 * 1'024;
    if (size > kMaximumFuzzInput) {
        return 0;
    }

    static ScratchLog scratch;
    const bool use_raw_input = size > 0 && (data[0] & 1U) != 0;
    const auto* payload = size > 0 ? data + 1 : data;
    const auto payload_size = size > 0 ? size - 1 : 0;
    const bool prepared =
        use_raw_input
            ? write_raw(scratch.path(), payload, payload_size)
            : write_mutated_valid_log(scratch.path(), payload, payload_size);
    if (prepared) {
        exercise_parser(scratch.path());
    }
    return 0;
}
