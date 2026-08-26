#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <exception>
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
#include "traceforge/generator/workload_generator.hpp"
#include "traceforge/generator/workload_publisher.hpp"
#include "traceforge/telemetry/collector_service.hpp"
#include "traceforge/telemetry/queued_sink.hpp"
#include "traceforge/version.hpp"

namespace {

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
           << "                          Generate deterministic sensor data\n";
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
        << "  --consumer-delay-us N    Artificial delay for overload testing\n";
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
            } else {
                throw std::invalid_argument("unknown collector option: " +
                                            std::string{option});
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Collector error: " << error.what() << '\n';
        return 2;
    }

    CountingConsumer consumer{options.consumer_delay};
    traceforge::telemetry::QueuedTelemetrySink sink{options.queue_capacity,
                                                    consumer};
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

    std::cout << "TraceForge collector listening on " << options.listen_address;
    if (options.listen_address.ends_with(":0")) {
        std::cout << " (selected port " << selected_port << ')';
    }
    std::cout << "; queue_capacity=" << options.queue_capacity
              << "; overload_policy=reject_stream\n";
    server->Wait();
    return 0;
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

    std::cerr << "Unknown argument: " << argument << "\n\n";
    print_help(std::cerr);
    return 2;
}
