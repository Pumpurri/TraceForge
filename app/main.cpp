#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include "traceforge/about.hpp"
#include "traceforge/analysis/analyzer.hpp"
#include "traceforge/generator/workload_generator.hpp"
#include "traceforge/generator/workload_publisher.hpp"
#include "traceforge/recording/log.hpp"
#include "traceforge/recording/log_consumer.hpp"
#include "traceforge/replay/replay.hpp"
#include "traceforge/telemetry/collector_service.hpp"
#include "traceforge/telemetry/queued_sink.hpp"
#include "traceforge/version.hpp"

namespace {

volatile std::sig_atomic_t g_shutdown_signal = 0;

void request_shutdown(int signal_number) {
    g_shutdown_signal = signal_number;
}

class CountingConsumer final : public traceforge::telemetry::TelemetryConsumer {
  public:
    explicit CountingConsumer(std::chrono::microseconds delay)
        : delay_(delay) {}

    bool consume(traceforge::telemetry::CollectedEvent event) override {
        static_cast<void>(event);
        if (delay_.count() > 0) {
            std::this_thread::sleep_for(delay_);
        }
        accepted_events_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

  private:
    std::chrono::microseconds delay_;
    std::atomic<std::uint64_t> accepted_events_{0};
};

struct CollectorOptions {
    std::string listen_address{"0.0.0.0:50051"};
    std::size_t queue_capacity{4'096};
    std::chrono::microseconds consumer_delay{0};
    std::filesystem::path output_path;
    std::uint64_t flush_every_records{64};
};

void print_help(std::ostream& output) {
    output << traceforge::project_name()
           << " - multi-sensor telemetry tooling\n\n"
           << "Usage:\n"
           << "  traceforge --help       Show this message\n"
           << "  traceforge --version    Show the current version\n"
           << "  traceforge collect [OPTIONS]\n"
           << "                          Run the telemetry collector\n"
           << "  traceforge generate [OPTIONS]\n"
           << "                          Generate deterministic sensor data\n"
           << "  traceforge inspect FILE Inspect and validate a telemetry log\n"
           << "  traceforge analyze FILE Summarize rates, jitter, gaps, and "
              "timing\n"
           << "  traceforge recover FILE Remove an incomplete final record\n"
           << "  traceforge replay FILE [OPTIONS]\n"
           << "                          Deterministically replay a log\n";
}

void print_generator_help(std::ostream& output) {
    output << "Generator options:\n"
           << "  --seed N                 Deterministic random seed\n"
           << "  --duration-ms N          Simulated duration in milliseconds\n"
           << "  --jitter-us N            Maximum signed timestamp jitter\n"
           << "  --clock-drift-ppm N      Source-clock drift in parts per "
              "million\n"
           << "  --drop-per-million N     Event drop probability\n"
           << "  --duplicate-per-million N  Event duplication probability\n"
           << "  --reorder-per-million N  Adjacent-pair reorder probability\n"
           << "  --target ADDRESS         Stream to a running collector\n"
           << "  --connect-timeout-ms N   Collector connection timeout\n"
           << "  --print-events           Print the generated event stream\n";
}

void print_collector_help(std::ostream& output) {
    output
        << "Collector options:\n"
        << "  --listen ADDRESS         Listen address (default 0.0.0.0:50051)\n"
        << "  --queue-capacity N       Maximum queued events (default 4096)\n"
        << "  --consumer-delay-us N    Artificial delay for overload testing\n"
        << "  --output FILE            Persist accepted events to a .tflog\n"
        << "  --flush-every N          Flush after N records (default 64)\n";
}

void print_replay_help(std::ostream& output) {
    output
        << "Replay options:\n"
        << "  --from-ns N              Inclusive collector timestamp seek\n"
        << "  --until-ns N             Exclusive collector timestamp limit\n"
        << "  --speed X                Wall-clock speed; 0 is immediate\n"
        << "  --print-events           Print each event in replay order\n";
}

template <typename Integer>
Integer parse_integer(std::string_view value, std::string_view option) {
    Integer parsed{};
    const auto* begin = value.data();
    const auto* end = begin + value.size();
    const auto [position, error] = std::from_chars(begin, end, parsed);
    if (error != std::errc{} || position != end) {
        throw std::invalid_argument("invalid value for " + std::string{option});
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

std::uint32_t parse_probability(std::string_view value,
                                std::string_view option) {
    const auto parsed = parse_integer<std::uint64_t>(value, option);
    if (parsed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("value is too large for " +
                                    std::string{option});
    }
    return static_cast<std::uint32_t>(parsed);
}

double parse_speed(std::string_view value) {
    const std::string text{value};
    std::size_t consumed = 0;
    const double parsed = std::stod(text, &consumed);
    if (consumed != text.size() || !std::isfinite(parsed) || parsed < 0.0) {
        throw std::invalid_argument(
            "replay speed must be finite and non-negative");
    }
    return parsed;
}

int run_generator(int argc, char* argv[]) {
    traceforge::generator::GeneratorConfig config;
    bool print_events = false;
    std::string target;
    std::chrono::milliseconds connect_timeout{5'000};

    try {
        for (int index = 2; index < argc; ++index) {
            const std::string_view option{argv[index]};
            if (option == "--help" || option == "-h") {
                print_generator_help(std::cout);
                return 0;
            }
            if (option == "--print-events") {
                print_events = true;
                continue;
            }

            const auto value = option_value(index, argc, argv);
            if (option == "--seed") {
                config.seed = parse_integer<std::uint64_t>(value, option);
            } else if (option == "--duration-ms") {
                const auto duration_ms =
                    parse_integer<std::int64_t>(value, option);
                if (duration_ms <= 0 || duration_ms > 3'600'000) {
                    throw std::invalid_argument(
                        "duration must be between 1 and 3600000 ms");
                }
                config.duration = std::chrono::milliseconds{duration_ms};
            } else if (option == "--jitter-us") {
                const auto jitter_us =
                    parse_integer<std::int64_t>(value, option);
                if (jitter_us < 0 || jitter_us > 3'600'000'000) {
                    throw std::invalid_argument(
                        "jitter must be between 0 and 3600000000 us");
                }
                config.max_jitter = std::chrono::microseconds{jitter_us};
            } else if (option == "--clock-drift-ppm") {
                config.clock_drift_ppm =
                    parse_integer<std::int64_t>(value, option);
            } else if (option == "--drop-per-million") {
                config.faults.drop_per_million =
                    parse_probability(value, option);
            } else if (option == "--duplicate-per-million") {
                config.faults.duplicate_per_million =
                    parse_probability(value, option);
            } else if (option == "--reorder-per-million") {
                config.faults.reorder_per_million =
                    parse_probability(value, option);
            } else if (option == "--target") {
                target = value;
            } else if (option == "--connect-timeout-ms") {
                const auto timeout_ms =
                    parse_integer<std::int64_t>(value, option);
                if (timeout_ms <= 0 || timeout_ms > 60'000) {
                    throw std::invalid_argument(
                        "connect timeout must be between 1 and 60000 ms");
                }
                connect_timeout = std::chrono::milliseconds{timeout_ms};
            } else {
                throw std::invalid_argument("unknown generator option: " +
                                            std::string{option});
            }
        }

        const auto workload =
            traceforge::generator::WorkloadGenerator{config}.generate();
        if (print_events) {
            for (const auto& event : workload.events) {
                std::cout << event.source_timestamp_ns() << ' '
                          << event.producer_id() << ' '
                          << event.sequence_number() << ' '
                          << traceforge::generator::payload_name(
                                 event.payload_case())
                          << '\n';
            }
        }

        std::cout << "seed=" << config.seed
                  << " generated=" << workload.stats.generated_events
                  << " emitted=" << workload.events.size()
                  << " dropped=" << workload.stats.dropped_events
                  << " duplicated=" << workload.stats.duplicated_events
                  << " reordered_pairs=" << workload.stats.reordered_pairs
                  << " hash="
                  << traceforge::generator::format_hash(workload.hash) << '\n';

        if (!target.empty()) {
            const traceforge::generator::WorkloadPublisher publisher{{
                .target = target,
                .connect_timeout = connect_timeout,
            }};
            const auto report = publisher.publish(workload.events);
            for (const auto& producer : report.producers) {
                std::cout << "producer=" << producer.producer_id
                          << " attempted=" << producer.attempted_events
                          << " written=" << producer.written_events
                          << " accepted=" << producer.accepted_events
                          << " gaps=" << producer.sequence_gaps
                          << " grpc_status=" << producer.grpc_status_code;
                if (!producer.error_message.empty()) {
                    std::cout << " error=\"" << producer.error_message << '"';
                }
                std::cout << '\n';
            }
            std::cout << "publish_target=" << target
                      << " producers=" << report.producers.size()
                      << " attempted=" << report.attempted_events()
                      << " accepted=" << report.accepted_events() << " result="
                      << (report.succeeded() ? "success" : "failed") << '\n';
            return report.succeeded() ? 0 : 3;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Generator error: " << error.what() << '\n';
        return 2;
    }
}

int run_collector(int argc, char* argv[]) {
    CollectorOptions options;
    try {
        for (int index = 2; index < argc; ++index) {
            const std::string_view option{argv[index]};
            if (option == "--help" || option == "-h") {
                print_collector_help(std::cout);
                return 0;
            }

            const auto value = option_value(index, argc, argv);
            if (option == "--listen") {
                options.listen_address = value;
            } else if (option == "--queue-capacity") {
                const auto capacity =
                    parse_integer<std::uint64_t>(value, option);
                if (capacity == 0 || capacity > 1'000'000) {
                    throw std::invalid_argument(
                        "queue capacity must be between 1 and 1000000");
                }
                options.queue_capacity = static_cast<std::size_t>(capacity);
            } else if (option == "--consumer-delay-us") {
                const auto delay = parse_integer<std::int64_t>(value, option);
                if (delay < 0 || delay > 10'000'000) {
                    throw std::invalid_argument(
                        "consumer delay must be between 0 and 10000000 us");
                }
                options.consumer_delay = std::chrono::microseconds{delay};
            } else if (option == "--output") {
                options.output_path = value;
                if (options.output_path.empty()) {
                    throw std::invalid_argument("output path cannot be empty");
                }
            } else if (option == "--flush-every") {
                const auto flush_every =
                    parse_integer<std::uint64_t>(value, option);
                if (flush_every == 0 || flush_every > 1'000'000) {
                    throw std::invalid_argument(
                        "flush interval must be between 1 and 1000000");
                }
                options.flush_every_records = flush_every;
            } else {
                throw std::invalid_argument("unknown collector option: " +
                                            std::string{option});
            }
        }
        if (!options.output_path.empty() &&
            options.consumer_delay.count() > 0) {
            throw std::invalid_argument(
                "consumer delay cannot be combined with file recording");
        }
    } catch (const std::exception& error) {
        std::cerr << "Collector error: " << error.what() << '\n';
        return 2;
    }

    std::unique_ptr<traceforge::telemetry::TelemetryConsumer> consumer;
    traceforge::recording::LogWriterConsumer* recorder = nullptr;
    if (options.output_path.empty()) {
        consumer = std::make_unique<CountingConsumer>(options.consumer_delay);
    } else {
        auto recording_consumer =
            std::make_unique<traceforge::recording::LogWriterConsumer>(
                options.output_path,
                traceforge::recording::LogWriterOptions{
                    .flush_every_records = options.flush_every_records,
                });
        if (!recording_consumer->good()) {
            std::cerr << "Collector error: " << recording_consumer->error()
                      << '\n';
            return 1;
        }
        recorder = recording_consumer.get();
        consumer = std::move(recording_consumer);
    }

    traceforge::telemetry::QueuedTelemetrySink sink{options.queue_capacity,
                                                    *consumer};
    traceforge::telemetry::CollectorService service{sink};
    grpc::ServerBuilder builder;
    int selected_port = 0;
    builder.AddListeningPort(options.listen_address,
                             grpc::InsecureServerCredentials(), &selected_port);
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "Failed to start collector on " << options.listen_address
                  << '\n';
        return 1;
    }

    g_shutdown_signal = 0;
    const auto previous_interrupt_handler =
        std::signal(SIGINT, request_shutdown);
    const auto previous_terminate_handler =
        std::signal(SIGTERM, request_shutdown);

    std::cout << "TraceForge collector listening on " << options.listen_address;
    if (options.listen_address.ends_with(":0")) {
        std::cout << " (selected port " << selected_port << ')';
    }
    std::cout << "; queue_capacity=" << options.queue_capacity
              << "; overload_policy=reject_stream";
    if (!options.output_path.empty()) {
        std::cout << "; output=" << options.output_path.string()
                  << "; flush_every=" << options.flush_every_records;
    }
    std::cout << std::endl;

    std::jthread signal_monitor{[&server](std::stop_token stop) {
        while (!stop.stop_requested() && g_shutdown_signal == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
        if (g_shutdown_signal != 0) {
            server->Shutdown();
        }
    }};

    server->Wait();
    signal_monitor.request_stop();
    sink.shutdown();
    if (recorder != nullptr && !recorder->flush()) {
        std::cerr << "Failed to flush recording: " << recorder->error() << '\n';
        return 1;
    }

    const auto stats = sink.stats();
    std::cout << "collector_shutdown="
              << (g_shutdown_signal == 0 ? "server" : "signal")
              << " signal=" << g_shutdown_signal
              << " accepted=" << stats.queue.accepted_items
              << " rejected=" << stats.queue.rejected_items
              << " consumed=" << stats.consumed_events
              << " queue_high_watermark=" << stats.queue.high_watermark
              << " consumer_failures=" << stats.consumer_failures << '\n';

    static_cast<void>(std::signal(SIGINT, previous_interrupt_handler));
    static_cast<void>(std::signal(SIGTERM, previous_terminate_handler));
    return 0;
}

int run_inspect(const std::filesystem::path& path) {
    const auto result = traceforge::recording::read_log(path);
    std::cout << "path=" << path.string()
              << " status=" << traceforge::recording::status_name(result.status)
              << " records=" << result.records.size()
              << " file_bytes=" << result.file_size
              << " valid_bytes=" << result.valid_bytes
              << " created_unix_ns=" << result.created_unix_ns;
    if (result.status != traceforge::recording::LogReadStatus::complete) {
        std::cout << " error_offset=" << result.error_offset << " message=\""
                  << result.message << '"';
    }
    std::cout << '\n';
    return result.status == traceforge::recording::LogReadStatus::complete ? 0
                                                                           : 3;
}

int run_recover(const std::filesystem::path& path) {
    const auto result = traceforge::recording::recover_truncated_tail(path);
    std::cout << "path=" << path.string()
              << " result=" << (result.succeeded ? "success" : "refused")
              << " changed=" << (result.changed ? "true" : "false")
              << " original_bytes=" << result.original_size
              << " recovered_bytes=" << result.recovered_size << " message=\""
              << result.message << "\"\n";
    return result.succeeded ? 0 : 3;
}

void print_optional_metric(std::ostream& output,
                           const std::optional<double>& metric) {
    if (metric.has_value()) {
        output << *metric;
    } else {
        output << "n/a";
    }
}

int run_analyze(const std::filesystem::path& path) {
    const auto report = traceforge::analysis::analyze_log(path);
    std::cout << "path=" << path.string()
              << " status=" << traceforge::analysis::status_name(report.status)
              << " records=" << report.records
              << " file_bytes=" << report.file_bytes;
    if (report.status != traceforge::analysis::AnalysisStatus::complete) {
        std::cout << " message=\"" << report.message << "\"\n";
        return 3;
    }

    std::cout << std::fixed << std::setprecision(3) << " duration_s=";
    print_optional_metric(std::cout, report.duration_seconds);
    std::cout << '\n';

    for (const auto& producer : report.producers) {
        std::cout << "producer=" << producer.producer_id
                  << " records=" << producer.records
                  << " sequence_gaps=" << producer.sequence_gaps
                  << " non_increasing_sequences="
                  << producer.non_increasing_sequences << '\n';
    }

    for (const auto& stream : report.streams) {
        std::cout << "stream=" << stream.producer_id << '/' << stream.payload
                  << " records=" << stream.records << " rate_hz=";
        print_optional_metric(std::cout, stream.observed_rate_hz);
        std::cout << " interarrival_p50_ms=";
        print_optional_metric(std::cout, stream.interarrival_p50_ms);
        std::cout << " jitter_p95_ms=";
        print_optional_metric(std::cout, stream.jitter_p95_ms);
        std::cout << " source_timestamp_regressions="
                  << stream.source_timestamp_regressions << '\n';
    }

    if (report.utc_arrival_delta.has_value()) {
        const auto& delta = *report.utc_arrival_delta;
        std::cout << "utc_arrival_delta samples=" << delta.samples
                  << " p50_ms=" << delta.p50_ms
                  << " p95_ms=" << delta.p95_ms
                  << " p99_ms=" << delta.p99_ms
                  << " max_ms=" << delta.maximum_ms << '\n';
    } else {
        std::cout << "utc_arrival_delta samples=0\n";
    }
    std::cout << "faults warnings=" << report.faults.warnings
              << " errors=" << report.faults.errors
              << " critical=" << report.faults.critical << '\n';
    return 0;
}

int run_replay(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: traceforge replay FILE [OPTIONS]\n";
        return 2;
    }

    traceforge::replay::ReplayOptions options;
    bool print_events = false;
    try {
        for (int index = 3; index < argc; ++index) {
            const std::string_view option{argv[index]};
            if (option == "--help" || option == "-h") {
                print_replay_help(std::cout);
                return 0;
            }
            if (option == "--print-events") {
                print_events = true;
                continue;
            }

            const auto value = option_value(index, argc, argv);
            if (option == "--from-ns") {
                options.from_collector_timestamp_ns =
                    parse_integer<std::int64_t>(value, option);
            } else if (option == "--until-ns") {
                options.until_collector_timestamp_ns =
                    parse_integer<std::int64_t>(value, option);
            } else if (option == "--speed") {
                options.speed = parse_speed(value);
            } else {
                throw std::invalid_argument("unknown replay option: " +
                                            std::string{option});
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Replay error: " << error.what() << '\n';
        return 2;
    }

    const traceforge::replay::ReplayEngine engine{argv[2]};
    const auto result = engine.replay(
        options,
        print_events
            ? traceforge::replay::ReplayConsumer{
                  [](const traceforge::recording::LogRecord& record) {
                      std::cout
                          << "collector_timestamp_ns="
                          << record.collector_arrival_timestamp_ns
                          << " record_index=" << record.record_index
                          << " source_timestamp_ns="
                          << record.event.source_timestamp_ns()
                          << " producer=" << record.event.producer_id()
                          << " sequence=" << record.event.sequence_number()
                          << " payload="
                          << traceforge::generator::payload_name(
                                 record.event.payload_case())
                          << '\n';
                      return true;
                  }}
            : traceforge::replay::ReplayConsumer{});

    std::cout << "path=" << argv[2]
              << " status=" << traceforge::replay::status_name(result.status)
              << " indexed=" << result.indexed_records
              << " replayed=" << result.replayed_records
              << " speed=" << options.speed
              << " hash=" << traceforge::replay::format_hash(result.hash);
    if (result.first_timestamp_ns.has_value()) {
        std::cout << " first_collector_timestamp_ns="
                  << *result.first_timestamp_ns
                  << " last_collector_timestamp_ns="
                  << *result.last_timestamp_ns;
    }
    if (result.status != traceforge::replay::ReplayStatus::complete) {
        std::cout << " message=\"" << result.message << '"';
    }
    std::cout << '\n';
    return result.status == traceforge::replay::ReplayStatus::complete ? 0 : 3;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        print_help(std::cout);
        return 0;
    }

    const std::string_view argument{argv[1]};
    if (argument == "--help" || argument == "-h") {
        print_help(std::cout);
        return 0;
    }

    if (argument == "--version") {
        std::cout << traceforge::project_name() << ' ' << traceforge::kVersion
                  << '\n';
        return 0;
    }

    if (argument == "collect") {
        return run_collector(argc, argv);
    }

    if (argument == "generate") {
        return run_generator(argc, argv);
    }

    if (argument == "inspect") {
        if (argc != 3) {
            std::cerr << "Usage: traceforge inspect FILE\n";
            return 2;
        }
        return run_inspect(argv[2]);
    }

    if (argument == "recover") {
        if (argc != 3) {
            std::cerr << "Usage: traceforge recover FILE\n";
            return 2;
        }
        return run_recover(argv[2]);
    }

    if (argument == "analyze") {
        if (argc != 3) {
            std::cerr << "Usage: traceforge analyze FILE\n";
            return 2;
        }
        return run_analyze(argv[2]);
    }

    if (argument == "replay") {
        return run_replay(argc, argv);
    }

    std::cerr << "Unknown argument: " << argument << "\n\n";
    print_help(std::cerr);
    return 2;
}
