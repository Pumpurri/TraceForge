#include "traceforge/generator/workload_publisher.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/status.h>

#include "traceforge/v1/telemetry.grpc.pb.h"

namespace traceforge::generator {
namespace {

struct ProducerEvents {
    std::string producer_id;
    std::vector<const v1::TelemetryEvent*> events;
};

[[nodiscard]] std::vector<ProducerEvents>
group_by_producer(std::span<const v1::TelemetryEvent> events) {
    std::map<std::string, std::vector<const v1::TelemetryEvent*>> grouped;
    for (const auto& event : events) {
        grouped[event.producer_id()].push_back(&event);
    }

    std::vector<ProducerEvents> producers;
    producers.reserve(grouped.size());
    for (auto& [producer_id, producer_events] : grouped) {
        producers.push_back({
            .producer_id = std::move(producer_id),
            .events = std::move(producer_events),
        });
    }
    return producers;
}

}  // namespace

bool ProducerPublishResult::succeeded() const noexcept {
    return grpc_status_code == static_cast<int>(grpc::StatusCode::OK) &&
           written_events == attempted_events &&
           accepted_events == attempted_events;
}

bool PublishReport::succeeded() const noexcept {
    return std::all_of(
        producers.begin(), producers.end(),
        [](const ProducerPublishResult& result) { return result.succeeded(); });
}

std::uint64_t PublishReport::attempted_events() const noexcept {
    return std::accumulate(
        producers.begin(), producers.end(), std::uint64_t{0},
        [](std::uint64_t total, const ProducerPublishResult& result) {
            return total + result.attempted_events;
        });
}

std::uint64_t PublishReport::accepted_events() const noexcept {
    return std::accumulate(
        producers.begin(), producers.end(), std::uint64_t{0},
        [](std::uint64_t total, const ProducerPublishResult& result) {
            return total + result.accepted_events;
        });
}

WorkloadPublisher::WorkloadPublisher(WorkloadPublisherConfig config)
    : config_(std::move(config)) {
    if (config_.target.empty()) {
        throw std::invalid_argument("publisher target cannot be empty");
    }
    if (config_.connect_timeout.count() <= 0) {
        throw std::invalid_argument("connect timeout must be positive");
    }
}

PublishReport
WorkloadPublisher::publish(std::span<const v1::TelemetryEvent> events) const {
    const auto producer_events = group_by_producer(events);
    PublishReport report;
    report.producers.resize(producer_events.size());
    if (producer_events.empty()) {
        return report;
    }

    auto channel =
        grpc::CreateChannel(config_.target, grpc::InsecureChannelCredentials());
    if (!channel->WaitForConnected(std::chrono::system_clock::now() +
                                   config_.connect_timeout)) {
        for (std::size_t index = 0; index < producer_events.size(); ++index) {
            report.producers[index] = {
                .producer_id = producer_events[index].producer_id,
                .attempted_events = producer_events[index].events.size(),
                .grpc_status_code =
                    static_cast<int>(grpc::StatusCode::UNAVAILABLE),
                .error_message = "collector connection timed out",
            };
        }
        return report;
    }

    std::vector<std::thread> threads;
    threads.reserve(producer_events.size());
    for (std::size_t index = 0; index < producer_events.size(); ++index) {
        threads.emplace_back([&, index] {
            const auto& producer = producer_events[index];
            ProducerPublishResult result;
            result.producer_id = producer.producer_id;
            result.attempted_events = producer.events.size();

            auto stub = v1::TelemetryCollector::NewStub(channel);
            grpc::ClientContext context;
            v1::StreamSummary summary;
            auto writer = stub->StreamTelemetry(&context, &summary);
            for (const auto* event : producer.events) {
                if (!writer->Write(*event)) {
                    break;
                }
                ++result.written_events;
            }
            writer->WritesDone();
            const auto status = writer->Finish();
            result.grpc_status_code = static_cast<int>(status.error_code());
            result.error_message = status.error_message();
            if (status.ok()) {
                result.accepted_events = summary.accepted_events();
                result.sequence_gaps = summary.sequence_gaps();
            }
            report.producers[index] = std::move(result);
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    return report;
}

}  // namespace traceforge::generator
