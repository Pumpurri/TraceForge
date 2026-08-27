#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#if defined(__APPLE__) || defined(__unix__)
#include <sys/resource.h>
#endif

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include "traceforge/generator/workload_publisher.hpp"
#include "traceforge/recording/log.hpp"
#include "traceforge/replay/replay.hpp"
#include "traceforge/telemetry/collector_service.hpp"
#include "traceforge/telemetry/queued_sink.hpp"
#include "traceforge/telemetry/validation.hpp"

namespace {

using SteadyClock = std::chrono::steady_clock;

struct BenchmarkOptions {
    std::size_t events{100'000};
    std::size_t producers{7};
    std::size_t queue_capacity{65'536};
};

class TemporaryLog {
  public:
    TemporaryLog() {
        const auto timestamp =
            SteadyClock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("traceforge_benchmark_" + std::to_string(timestamp) +
                 ".tflog");
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

class LatencyConsumer final
    : public traceforge::telemetry::TelemetryConsumer {
  public:
    explicit LatencyConsumer(std::size_t expected_events) {
        latencies_ns_.reserve(expected_events);
    }

    bool consume(traceforge::telemetry::CollectedEvent event) override {
        const auto now =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        latencies_ns_.push_back(
            std::max<std::int64_t>(
                0, now - event.collector_arrival_timestamp_ns));
        return true;
    }

    [[nodiscard]] std::span<const std::int64_t> latencies_ns() const noexcept {
        return latencies_ns_;
    }

  private:
    std::vector<std::int64_t> latencies_ns_;
};

template <typename Integer>
Integer parse_integer(std::string_view value, std::string_view option) {
    Integer parsed{};
    const auto* begin = value.data();
    const auto* end = begin + value.size();
    const auto [position, error] = std::from_chars(begin, end, parsed);
    if (error != std::errc{} || position != end) {
        throw std::invalid_argument("invalid value for " +
                                    std::string{option});
    }
    return parsed;
}

std::string_view option_value(int& index, int argc, char* argv[]) {
    if (index + 1 >= argc) {
        throw std::invalid_argument("missing value after " +
                                    std::string{argv[index]});
    }
    ++index;
    return argv[index];
}

BenchmarkOptions parse_options(int argc, char* argv[]) {
    BenchmarkOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view option{argv[index]};
        if (option == "--help" || option == "-h") {
            std::cout
                << "Usage: traceforge_benchmark [OPTIONS]\n"
                << "  --events N          Events to ingest and replay (default "
                << options.events << ")\n"
                << "  --producers N       Concurrent gRPC producers (default "
                << options.producers << ")\n"
                << "  --queue-capacity N  Bounded ingestion capacity (default "
                << options.queue_capacity << ")\n";
            std::exit(0);
        }

        const auto value = option_value(index, argc, argv);
        if (option == "--events") {
            options.events = parse_integer<std::size_t>(value, option);
        } else if (option == "--producers") {
            options.producers = parse_integer<std::size_t>(value, option);
        } else if (option == "--queue-capacity") {
            options.queue_capacity =
                parse_integer<std::size_t>(value, option);
        } else {
            throw std::invalid_argument("unknown benchmark option: " +
                                        std::string{option});
        }
    }

    if (options.events == 0 || options.events > 2'000'000) {
        throw std::invalid_argument(
            "events must be between 1 and 2000000");
    }
    if (options.producers == 0 || options.producers > 64 ||
        options.producers > options.events) {
        throw std::invalid_argument(
            "producers must be between 1 and 64 and no greater than events");
    }
    if (options.queue_capacity == 0 ||
        options.queue_capacity > 2'000'000) {
        throw std::invalid_argument(
            "queue capacity must be between 1 and 2000000");
    }
    return options;
}

std::vector<traceforge::v1::TelemetryEvent>
make_events(const BenchmarkOptions& options) {
    constexpr std::int64_t kBaseTimestampNs =
        1'700'000'000'000'000'000;
    std::vector<traceforge::v1::TelemetryEvent> events;
    events.reserve(options.events);
    for (std::size_t index = 0; index < options.events; ++index) {
        const auto producer = index % options.producers;
        const auto sequence = index / options.producers;
        traceforge::v1::TelemetryEvent event;
        event.set_schema_version(
            traceforge::telemetry::kTelemetrySchemaVersion);
        event.set_producer_id("benchmark-" + std::to_string(producer));
        event.set_sequence_number(static_cast<std::uint64_t>(sequence));
        event.set_source_timestamp_ns(
            kBaseTimestampNs + static_cast<std::int64_t>(index));
        event.set_source_clock(traceforge::v1::CLOCK_DOMAIN_UTC);
        auto* camera = event.mutable_camera();
        camera->set_frame_number(static_cast<std::uint64_t>(sequence));
        camera->set_width_pixels(1920);
        camera->set_height_pixels(1080);
        camera->set_exposure_time_ns(8'000'000);
        events.push_back(std::move(event));
    }
    return events;
}

[[nodiscard]] double elapsed_seconds(SteadyClock::time_point begin,
                                     SteadyClock::time_point end) {
    return std::chrono::duration<double>{end - begin}.count();
}

[[nodiscard]] double process_cpu_seconds(std::clock_t begin,
                                         std::clock_t end) {
    return static_cast<double>(end - begin) /
           static_cast<double>(CLOCKS_PER_SEC);
}

[[nodiscard]] std::uint64_t peak_resident_bytes() {
#if defined(__APPLE__) || defined(__unix__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
#if defined(__APPLE__)
    return static_cast<std::uint64_t>(usage.ru_maxrss);
#else
    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1'024U;
#endif
#else
    return 0;
#endif
}

[[nodiscard]] double percentile_us(std::span<const std::int64_t> values,
                                   double quantile) {
    if (values.empty()) {
        return 0.0;
    }
    std::vector<std::int64_t> sorted{values.begin(), values.end()};
    std::sort(sorted.begin(), sorted.end());
    const auto rank = static_cast<std::size_t>(std::ceil(
        quantile * static_cast<double>(sorted.size())));
    const auto index = std::min(sorted.size() - 1,
                                rank == 0 ? std::size_t{0} : rank - 1);
    return static_cast<double>(sorted[index]) / 1'000.0;
}

bool benchmark_ingestion(
    const BenchmarkOptions& options,
    std::span<const traceforge::v1::TelemetryEvent> events) {
    LatencyConsumer consumer{events.size()};
    traceforge::telemetry::QueuedTelemetrySink sink{options.queue_capacity,
                                                    consumer};
    traceforge::telemetry::CollectorService service{sink};
    grpc::ServerBuilder builder;
    int selected_port = 0;
    builder.AddListeningPort("127.0.0.1:0",
                             grpc::InsecureServerCredentials(),
                             &selected_port);
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    if (!server || selected_port == 0) {
        std::cerr << "benchmark error: failed to start loopback collector\n";
        return false;
    }

    const traceforge::generator::WorkloadPublisher publisher{{
        .target = "127.0.0.1:" + std::to_string(selected_port),
        .connect_timeout = std::chrono::seconds{5},
    }};
    const auto cpu_begin = std::clock();
    const auto wall_begin = SteadyClock::now();
    const auto report = publisher.publish(events);
    server->Shutdown();
    sink.shutdown();
    const auto wall_end = SteadyClock::now();
    const auto cpu_end = std::clock();

    if (!report.succeeded() || report.accepted_events() != events.size()) {
        std::cerr << "benchmark error: ingestion rejected events; increase "
                     "--queue-capacity\n";
        return false;
    }
    const auto stats = sink.stats();
    if (consumer.latencies_ns().size() != events.size() ||
        stats.consumed_events != events.size()) {
        std::cerr << "benchmark error: accepted events were not drained\n";
        return false;
    }

    const double wall_seconds = elapsed_seconds(wall_begin, wall_end);
    const double cpu_seconds = process_cpu_seconds(cpu_begin, cpu_end);
    std::cout << std::fixed << std::setprecision(2)
              << "benchmark=loopback_ingestion"
              << " events=" << events.size()
              << " producers=" << options.producers
              << " queue_capacity=" << options.queue_capacity
              << " wall_ms=" << wall_seconds * 1'000.0
              << " events_per_sec="
              << static_cast<double>(events.size()) / wall_seconds
              << " queue_latency_p50_us="
              << percentile_us(consumer.latencies_ns(), 0.50)
              << " queue_latency_p95_us="
              << percentile_us(consumer.latencies_ns(), 0.95)
              << " queue_latency_p99_us="
              << percentile_us(consumer.latencies_ns(), 0.99)
              << " cpu_seconds=" << cpu_seconds
              << " cpu_utilization_percent="
              << cpu_seconds / wall_seconds * 100.0
              << " queue_high_watermark=" << stats.queue.high_watermark
              << " rejected=" << stats.queue.rejected_items
              << " peak_rss_mib="
              << static_cast<double>(peak_resident_bytes()) /
                     (1'024.0 * 1'024.0)
              << '\n';
    return true;
}

bool benchmark_replay(
    std::span<const traceforge::v1::TelemetryEvent> events) {
    TemporaryLog log;
    {
        traceforge::recording::LogWriter writer{
            log.path(), {.flush_every_records = 1'024}};
        if (!writer.good()) {
            std::cerr << "benchmark error: " << writer.error() << '\n';
            return false;
        }
        constexpr std::int64_t kBaseArrivalNs =
            1'700'000'100'000'000'000;
        for (std::size_t index = 0; index < events.size(); ++index) {
            if (!writer.append(
                    events[index],
                    kBaseArrivalNs + static_cast<std::int64_t>(index))) {
                std::cerr << "benchmark error: " << writer.error() << '\n';
                return false;
            }
        }
        if (!writer.flush()) {
            std::cerr << "benchmark error: " << writer.error() << '\n';
            return false;
        }
    }

    const auto index_cpu_begin = std::clock();
    const auto index_wall_begin = SteadyClock::now();
    const traceforge::replay::ReplayEngine engine{log.path(), events.size()};
    const auto index_wall_end = SteadyClock::now();
    const auto index_cpu_end = std::clock();
    if (!engine.good()) {
        std::cerr << "benchmark error: " << engine.error() << '\n';
        return false;
    }

    const auto replay_cpu_begin = std::clock();
    const auto replay_wall_begin = SteadyClock::now();
    const auto result = engine.replay();
    const auto replay_wall_end = SteadyClock::now();
    const auto replay_cpu_end = std::clock();
    if (result.status != traceforge::replay::ReplayStatus::complete ||
        result.replayed_records != events.size()) {
        std::cerr << "benchmark error: replay did not complete\n";
        return false;
    }

    const double index_seconds =
        elapsed_seconds(index_wall_begin, index_wall_end);
    const double replay_seconds =
        elapsed_seconds(replay_wall_begin, replay_wall_end);
    std::cout << std::fixed << std::setprecision(2)
              << "benchmark=indexed_replay"
              << " records=" << result.replayed_records
              << " file_mib="
              << static_cast<double>(std::filesystem::file_size(log.path())) /
                     (1'024.0 * 1'024.0)
              << " index_ms=" << index_seconds * 1'000.0
              << " index_records_per_sec="
              << static_cast<double>(events.size()) / index_seconds
              << " index_cpu_seconds="
              << process_cpu_seconds(index_cpu_begin, index_cpu_end)
              << " replay_ms=" << replay_seconds * 1'000.0
              << " replay_records_per_sec="
              << static_cast<double>(events.size()) / replay_seconds
              << " replay_cpu_seconds="
              << process_cpu_seconds(replay_cpu_begin, replay_cpu_end)
              << " hash=" << traceforge::replay::format_hash(result.hash)
              << " peak_rss_mib="
              << static_cast<double>(peak_resident_bytes()) /
                     (1'024.0 * 1'024.0)
              << '\n';
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const auto options = parse_options(argc, argv);
        const auto events = make_events(options);
        if (!benchmark_ingestion(options, events) ||
            !benchmark_replay(events)) {
            return 1;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "benchmark error: " << error.what() << '\n';
        return 2;
    }
}
