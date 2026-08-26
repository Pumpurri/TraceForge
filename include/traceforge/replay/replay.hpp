#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "traceforge/recording/log.hpp"

namespace traceforge::replay {

struct ReplayOptions {
    std::optional<std::int64_t> from_collector_timestamp_ns;
    std::optional<std::int64_t> until_collector_timestamp_ns;
    double speed{0.0};
};

enum class ReplayStatus {
    complete,
    invalid_options,
    log_error,
    io_error,
    consumer_error,
};

struct ReplayResult {
    ReplayStatus status{ReplayStatus::log_error};
    std::uint64_t indexed_records{0};
    std::uint64_t replayed_records{0};
    std::uint64_t hash{0};
    std::optional<std::int64_t> first_timestamp_ns;
    std::optional<std::int64_t> last_timestamp_ns;
    std::string message;
};

using ReplayConsumer =
    std::function<bool(const recording::LogRecord& record)>;

class ReplayEngine {
  public:
    explicit ReplayEngine(std::filesystem::path path,
                          std::size_t maximum_records = 1'000'000);

    [[nodiscard]] bool good() const noexcept;
    [[nodiscard]] std::string error() const;
    [[nodiscard]] std::span<const recording::LogIndexEntry>
    index() const noexcept;
    [[nodiscard]] ReplayResult replay(
        ReplayOptions options = {}, ReplayConsumer consumer = {}) const;

  private:
    std::filesystem::path path_;
    std::vector<recording::LogIndexEntry> index_;
    std::string error_;
};

[[nodiscard]] std::string_view status_name(ReplayStatus status) noexcept;
[[nodiscard]] std::string format_hash(std::uint64_t hash);

}  // namespace traceforge::replay
