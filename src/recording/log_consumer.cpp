#include "traceforge/recording/log_consumer.hpp"

#include <utility>

namespace traceforge::recording {

LogWriterConsumer::LogWriterConsumer(std::filesystem::path path,
                                     LogWriterOptions options)
    : writer_(std::move(path), options) {}

bool LogWriterConsumer::consume(telemetry::CollectedEvent event) {
    return writer_.append(event.event, event.collector_arrival_timestamp_ns);
}

bool LogWriterConsumer::flush() { return writer_.flush(); }

bool LogWriterConsumer::good() const noexcept { return writer_.good(); }

std::string LogWriterConsumer::error() const { return writer_.error(); }

std::uint64_t LogWriterConsumer::record_count() const noexcept {
    return writer_.record_count();
}

std::uint64_t LogWriterConsumer::bytes_written() const noexcept {
    return writer_.bytes_written();
}

}  // namespace traceforge::recording
