#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <set>
#include <string>

#include <grpcpp/security/credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/status_code_enum.h>

#include "traceforge/generator/workload_generator.hpp"
#include "traceforge/generator/workload_publisher.hpp"
#include "traceforge/telemetry/collector_service.hpp"
#include "traceforge/telemetry/queued_sink.hpp"

namespace {

class RecordingConsumer final
    : public traceforge::telemetry::TelemetryConsumer {
  public:
    bool consume(traceforge::telemetry::CollectedEvent event) override {
        std::lock_guard lock{mutex_};
        ++event_count_;
        producer_ids_.insert(event.event.producer_id());
        return true;
    }

    [[nodiscard]] std::uint64_t event_count() const {
        std::lock_guard lock{mutex_};
        return event_count_;
    }

    [[nodiscard]] std::size_t producer_count() const {
        std::lock_guard lock{mutex_};
        return producer_ids_.size();
    }

  private:
    mutable std::mutex mutex_;
    std::set<std::string> producer_ids_;
    std::uint64_t event_count_{0};
};

}  // namespace

int main() {
    RecordingConsumer consumer;
    traceforge::telemetry::QueuedTelemetrySink sink{1'024, consumer};
    traceforge::telemetry::CollectorService service{sink};
    grpc::ServerBuilder builder;
    int selected_port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(),
                             &selected_port);
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    if (!server || selected_port == 0) {
        std::cerr << "Failed to start workload publisher test collector\n";
        return 1;
    }

    traceforge::generator::GeneratorConfig generator_config;
    generator_config.seed = 42;
    generator_config.duration = std::chrono::seconds{1};
    const auto workload =
        traceforge::generator::WorkloadGenerator{generator_config}.generate();
    const traceforge::generator::WorkloadPublisher publisher{{
        .target = "127.0.0.1:" + std::to_string(selected_port),
        .connect_timeout = std::chrono::seconds{5},
    }};

    const auto first_report = publisher.publish(workload.events);
    const auto reconnect_report = publisher.publish(workload.events);
    if (!first_report.succeeded() || !reconnect_report.succeeded() ||
        first_report.producers.size() != 7 ||
        first_report.attempted_events() != workload.events.size() ||
        first_report.accepted_events() != workload.events.size() ||
        reconnect_report.accepted_events() != workload.events.size()) {
        std::cerr << "Concurrent synthetic producers were not fully accepted\n";
        server->Shutdown();
        return 1;
    }

    generator_config.faults.duplicate_per_million = 1'000'000;
    const auto duplicate_workload =
        traceforge::generator::WorkloadGenerator{generator_config}.generate();
    const auto rejected_report = publisher.publish(duplicate_workload.events);
    if (rejected_report.succeeded()) {
        std::cerr << "Duplicate sequence faults were unexpectedly accepted\n";
        server->Shutdown();
        return 1;
    }
    for (const auto& producer : rejected_report.producers) {
        if (producer.grpc_status_code !=
            static_cast<int>(grpc::StatusCode::INVALID_ARGUMENT)) {
            std::cerr << "Publisher did not surface a collector rejection\n";
            server->Shutdown();
            return 1;
        }
    }

    server->Shutdown();
    sink.shutdown();
    const auto minimum_consumed = workload.events.size() * 2;
    if (consumer.event_count() < minimum_consumed ||
        consumer.producer_count() != 7 ||
        sink.stats().queue.high_watermark > sink.stats().queue.capacity) {
        std::cerr << "Queued collector lost accepted synthetic events\n";
        return 1;
    }

    return 0;
}
