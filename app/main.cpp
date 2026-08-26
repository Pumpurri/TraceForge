#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include "traceforge/about.hpp"
#include "traceforge/telemetry/collector_service.hpp"
#include "traceforge/version.hpp"

namespace {

class CountingSink final : public traceforge::telemetry::TelemetrySink {
  public:
    traceforge::telemetry::SinkResult try_accept(
        traceforge::telemetry::CollectedEvent event) override {
        static_cast<void>(event);
        accepted_events_.fetch_add(1, std::memory_order_relaxed);
        return traceforge::telemetry::SinkResult::accepted;
    }

  private:
    std::atomic<std::uint64_t> accepted_events_{0};
};

void print_help(std::ostream& output) {
    output << traceforge::project_name() << " - multi-sensor telemetry tooling\n\n"
           << "Usage:\n"
           << "  traceforge --help       Show this message\n"
           << "  traceforge --version    Show the current version\n"
           << "  traceforge collect [--listen ADDRESS]\n"
           << "                          Run the telemetry collector\n";
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

    std::cerr << "Unknown argument: " << argument << "\n\n";
    print_help(std::cerr);
    return 2;
}
