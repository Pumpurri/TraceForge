#pragma once

#include <filesystem>
#include <string>

#include "traceforge/recording/log.hpp"
#include "traceforge/telemetry/queued_sink.hpp"

namespace traceforge::recording {

class LogWriterConsumer final : public telemetry::TelemetryConsumer {
  public:
    explicit LogWriterConsumer(std::filesystem::path path,
                               LogWriterOptions options = {});

    bool consume(telemetry::CollectedEvent event) override;
    [[nodiscard]] bool flush();
    [[nodiscard]] bool good() const noexcept;
    [[nodiscard]] std::string error() const;
    [[nodiscard]] std::uint64_t record_count() const noexcept;
    [[nodiscard]] std::uint64_t bytes_written() const noexcept;

  private:
    LogWriter writer_;
};

}  // namespace traceforge::recording
