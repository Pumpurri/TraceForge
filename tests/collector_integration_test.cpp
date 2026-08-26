#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
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
    traceforge::telemetry::SinkResult try_accept(
        traceforge::telemetry::CollectedEvent event) override {
        std::lock_guard lock{mutex_};
        events_.push_back(std::move(event));
        return traceforge::telemetry::SinkResult::accepted;
    }

    [[nodiscard]] std::vector<traceforge::telemetry::CollectedEvent> events()
        const {
        std::lock_guard lock{mutex_};
        return events_;
    }

  private:
    mutable std::mutex mutex_;
    std::vector<traceforge::telemetry::CollectedEvent> events_;
};

traceforge::v1::TelemetryEvent make_event(std::uint64_t sequence_number) {
    traceforge::v1::TelemetryEvent event;
    event.set_schema_version(traceforge::telemetry::kTelemetrySchemaVersion);
    event.set_producer_id("integration-phone");
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
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
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
    auto invalid_writer = stub->StreamTelemetry(&invalid_context, &invalid_summary);
    auto invalid_event = make_event(10);
    invalid_event.set_schema_version(999);
    static_cast<void>(invalid_writer->Write(invalid_event));
    invalid_writer->WritesDone();
    const grpc::Status invalid_status = invalid_writer->Finish();
    server->Shutdown();

    if (invalid_status.error_code() != grpc::StatusCode::INVALID_ARGUMENT ||
        invalid_status.error_message().find("schema version") ==
            std::string::npos) {
        std::cerr << "Invalid network event did not return INVALID_ARGUMENT\n";
        return 1;
    }

    return 0;
}
