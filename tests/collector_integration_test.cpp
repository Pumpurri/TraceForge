#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include "traceforge/telemetry/collector_service.hpp"
#include "traceforge/telemetry/validation.hpp"
#include "traceforge/v1/telemetry.grpc.pb.h"

namespace {

class RecordingSink final : public traceforge::telemetry::TelemetrySink {
  public:
    traceforge::telemetry::SinkResult
    try_accept(traceforge::telemetry::CollectedEvent event) override {
        std::lock_guard lock{mutex_};
        events_.push_back(std::move(event));
        return traceforge::telemetry::SinkResult::accepted;
    }

    [[nodiscard]] std::vector<traceforge::telemetry::CollectedEvent>
    events() const {
        std::lock_guard lock{mutex_};
        return events_;
    }

  private:
    mutable std::mutex mutex_;
    std::vector<traceforge::telemetry::CollectedEvent> events_;
};

class AlwaysOverloadedSink final : public traceforge::telemetry::TelemetrySink {
  public:
    traceforge::telemetry::SinkResult
    try_accept(traceforge::telemetry::CollectedEvent event) override {
        static_cast<void>(event);
        return traceforge::telemetry::SinkResult::overloaded;
    }
};

traceforge::v1::TelemetryEvent
make_event(std::uint64_t sequence_number,
           std::string producer_id = "integration-phone") {
    traceforge::v1::TelemetryEvent event;
    event.set_schema_version(traceforge::telemetry::kTelemetrySchemaVersion);
    event.set_producer_id(std::move(producer_id));
    event.set_sequence_number(sequence_number);
    event.set_source_timestamp_ns(1'777'000'000'000'000'000 +
                                  static_cast<std::int64_t>(sequence_number));
    event.set_source_clock(traceforge::v1::CLOCK_DOMAIN_UTC);
    event.mutable_gyroscope()->set_x(0.1);
    event.mutable_gyroscope()->set_y(0.2);
    event.mutable_gyroscope()->set_z(0.3);
    return event;
}

}  // namespace

int main() {
    RecordingSink sink;
    traceforge::telemetry::CollectorService service{sink};
    grpc::ServerBuilder builder;
    int selected_port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(),
                             &selected_port);
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    if (!server || selected_port == 0) {
        std::cerr << "Failed to start loopback collector\n";
        return 1;
    }

    const auto address = "127.0.0.1:" + std::to_string(selected_port);
    auto channel =
        grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    if (!channel->WaitForConnected(std::chrono::system_clock::now() +
                                   std::chrono::seconds{5})) {
        std::cerr << "Loopback channel did not connect\n";
        server->Shutdown();
        return 1;
    }

    auto stub = traceforge::v1::TelemetryCollector::NewStub(channel);
    grpc::ClientContext context;
    traceforge::v1::StreamSummary summary;
    auto writer = stub->StreamTelemetry(&context, &summary);
    const bool writes_succeeded = writer->Write(make_event(5)) &&
                                  writer->Write(make_event(6)) &&
                                  writer->Write(make_event(9));
    writer->WritesDone();
    const grpc::Status status = writer->Finish();

    if (!writes_succeeded || !status.ok()) {
        std::cerr << "Stream failed: " << status.error_message() << '\n';
        server->Shutdown();
        return 1;
    }
    if (summary.producer_id() != "integration-phone" ||
        summary.accepted_events() != 3 || summary.sequence_gaps() != 2) {
        std::cerr << "Collector returned an invalid summary\n";
        server->Shutdown();
        return 1;
    }

    const auto events = sink.events();
    if (events.size() != 3 ||
        events.front().collector_arrival_timestamp_ns <= 0 ||
        events.back().event.sequence_number() != 9) {
        std::cerr << "Collector sink did not receive the expected events\n";
        server->Shutdown();
        return 1;
    }

    grpc::ClientContext invalid_context;
    traceforge::v1::StreamSummary invalid_summary;
    auto invalid_writer =
        stub->StreamTelemetry(&invalid_context, &invalid_summary);
    auto invalid_event = make_event(10);
    invalid_event.set_schema_version(999);
    static_cast<void>(invalid_writer->Write(invalid_event));
    invalid_writer->WritesDone();
    const grpc::Status invalid_status = invalid_writer->Finish();

    if (invalid_status.error_code() != grpc::StatusCode::INVALID_ARGUMENT ||
        invalid_status.error_message().find("schema version") ==
            std::string::npos) {
        std::cerr << "Invalid network event did not return INVALID_ARGUMENT\n";
        server->Shutdown();
        return 1;
    }

    constexpr int kConcurrentProducers = 6;
    constexpr int kEventsPerProducer = 100;
    std::vector<int> producer_results(kConcurrentProducers, 0);
    std::vector<std::thread> producer_threads;
    for (int producer = 0; producer < kConcurrentProducers; ++producer) {
        producer_threads.emplace_back([&, producer] {
            auto producer_stub =
                traceforge::v1::TelemetryCollector::NewStub(grpc::CreateChannel(
                    address, grpc::InsecureChannelCredentials()));
            grpc::ClientContext producer_context;
            traceforge::v1::StreamSummary producer_summary;
            auto producer_writer = producer_stub->StreamTelemetry(
                &producer_context, &producer_summary);
            bool wrote_all = true;
            const auto producer_id = "concurrent-" + std::to_string(producer);
            for (int sequence = 0; sequence < kEventsPerProducer; ++sequence) {
                wrote_all =
                    producer_writer->Write(make_event(
                        static_cast<std::uint64_t>(sequence), producer_id)) &&
                    wrote_all;
            }
            producer_writer->WritesDone();
            const auto producer_status = producer_writer->Finish();
            if (wrote_all && producer_status.ok() &&
                producer_summary.accepted_events() == kEventsPerProducer &&
                producer_summary.sequence_gaps() == 0) {
                producer_results[static_cast<std::size_t>(producer)] = 1;
            }
        });
    }
    for (auto& producer_thread : producer_threads) {
        producer_thread.join();
    }
    for (const auto result : producer_results) {
        if (result != 1) {
            std::cerr << "A concurrent producer stream failed\n";
            server->Shutdown();
            return 1;
        }
    }

    const auto concurrent_events = sink.events();
    constexpr std::size_t kExpectedEventCount =
        3 + kConcurrentProducers * kEventsPerProducer;
    if (concurrent_events.size() != kExpectedEventCount) {
        std::cerr << "Concurrent producer events were lost or corrupted\n";
        server->Shutdown();
        return 1;
    }

    grpc::ClientContext reconnect_context;
    traceforge::v1::StreamSummary reconnect_summary;
    auto reconnect_writer =
        stub->StreamTelemetry(&reconnect_context, &reconnect_summary);
    const bool reconnect_write =
        reconnect_writer->Write(make_event(0, "concurrent-0"));
    reconnect_writer->WritesDone();
    const auto reconnect_status = reconnect_writer->Finish();
    server->Shutdown();

    if (!reconnect_write || !reconnect_status.ok() ||
        reconnect_summary.accepted_events() != 1) {
        std::cerr << "Producer reconnect was not accepted\n";
        return 1;
    }

    AlwaysOverloadedSink overloaded_sink;
    traceforge::telemetry::CollectorService overloaded_service{overloaded_sink};
    grpc::ServerBuilder overloaded_builder;
    int overloaded_port = 0;
    overloaded_builder.AddListeningPort(
        "127.0.0.1:0", grpc::InsecureServerCredentials(), &overloaded_port);
    overloaded_builder.RegisterService(&overloaded_service);
    auto overloaded_server = overloaded_builder.BuildAndStart();
    if (!overloaded_server || overloaded_port == 0) {
        std::cerr << "Failed to start overload test collector\n";
        return 1;
    }

    auto overloaded_stub = traceforge::v1::TelemetryCollector::NewStub(
        grpc::CreateChannel("127.0.0.1:" + std::to_string(overloaded_port),
                            grpc::InsecureChannelCredentials()));
    grpc::ClientContext overloaded_context;
    traceforge::v1::StreamSummary overloaded_summary;
    auto overloaded_writer = overloaded_stub->StreamTelemetry(
        &overloaded_context, &overloaded_summary);
    static_cast<void>(overloaded_writer->Write(make_event(0)));
    overloaded_writer->WritesDone();
    const auto overloaded_status = overloaded_writer->Finish();
    overloaded_server->Shutdown();

    if (overloaded_status.error_code() !=
            grpc::StatusCode::RESOURCE_EXHAUSTED ||
        overloaded_status.error_message().find("fixed capacity") ==
            std::string::npos) {
        std::cerr << "Overload did not return RESOURCE_EXHAUSTED\n";
        return 1;
    }

    return 0;
}
