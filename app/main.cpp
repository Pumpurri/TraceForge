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

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include "traceforge/about.hpp"
#include "traceforge/generator/workload_generator.hpp"
#include "traceforge/telemetry/collector_service.hpp"
#include "traceforge/version.hpp"

namespace {

class CountingSink final : public traceforge::telemetry::TelemetrySink {
  public:
    traceforge::telemetry::SinkResult
    try_accept(traceforge::telemetry::CollectedEvent event) override {
        static_cast<void>(event);
        accepted_events_.fetch_add(1, std::memory_order_relaxed);
        return traceforge::telemetry::SinkResult::accepted;
    }

  private:
    std::atomic<std::uint64_t> accepted_events_{0};
};

void print_help(std::ostream& output) {
    output << traceforge::project_name()
           << " - multi-sensor telemetry tooling\n\n"
           << "Usage:\n"
           << "  traceforge --help       Show this message\n"
           << "  traceforge --version    Show the current version\n"
           << "  traceforge collect [--listen ADDRESS]\n"
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
           << "  --print-events           Print the generated event stream\n";
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
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Generator error: " << error.what() << '\n';
        return 2;
    }
}

int run_collector(std::string listen_address) {
    CountingSink sink;
    traceforge::telemetry::CollectorService service{sink};
    grpc::ServerBuilder builder;
    int selected_port = 0;
    builder.AddListeningPort(listen_address, grpc::InsecureServerCredentials(),
                             &selected_port);
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "Failed to start collector on " << listen_address << '\n';
        return 1;
    }

    std::cout << "TraceForge collector listening on " << listen_address;
    if (listen_address.ends_with(":0")) {
        std::cout << " (selected port " << selected_port << ')';
    }
    std::cout << '\n';
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
        std::string listen_address{"0.0.0.0:50051"};
        if (argc == 4 && std::string_view{argv[2]} == "--listen") {
            listen_address = argv[3];
        } else if (argc != 2) {
            std::cerr << "Invalid collector arguments\n\n";
            print_help(std::cerr);
            return 2;
        }
        return run_collector(std::move(listen_address));
    }

    if (argument == "generate") {
        return run_generator(argc, argv);
    }

    std::cerr << "Unknown argument: " << argument << "\n\n";
    print_help(std::cerr);
    return 2;
}
