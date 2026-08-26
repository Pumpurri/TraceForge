#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>

#include "traceforge/generator/workload_generator.hpp"
#include "traceforge/telemetry/validation.hpp"

namespace {

struct PayloadCounts {
    std::uint64_t accelerometer{0};
    std::uint64_t gyroscope{0};
    std::uint64_t gps{0};
    std::uint64_t temperature{0};
    std::uint64_t camera{0};
    std::uint64_t controller{0};
    std::uint64_t fault{0};
};

PayloadCounts
count_payloads(const traceforge::generator::GeneratedWorkload& workload) {
    PayloadCounts counts;
    for (const auto& event : workload.events) {
        switch (event.payload_case()) {
        case traceforge::v1::TelemetryEvent::kAccelerometer:
            ++counts.accelerometer;
            break;
        case traceforge::v1::TelemetryEvent::kGyroscope:
            ++counts.gyroscope;
            break;
        case traceforge::v1::TelemetryEvent::kGps:
            ++counts.gps;
            break;
        case traceforge::v1::TelemetryEvent::kTemperature:
            ++counts.temperature;
            break;
        case traceforge::v1::TelemetryEvent::kCamera:
            ++counts.camera;
            break;
        case traceforge::v1::TelemetryEvent::kControllerStatus:
            ++counts.controller;
            break;
        case traceforge::v1::TelemetryEvent::kFault:
            ++counts.fault;
            break;
        case traceforge::v1::TelemetryEvent::PAYLOAD_NOT_SET:
            break;
        }
    }
    return counts;
}

bool same_events(const traceforge::generator::GeneratedWorkload& left,
                 const traceforge::generator::GeneratedWorkload& right) {
    if (left.events.size() != right.events.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.events.size(); ++index) {
        if (left.events[index].SerializeAsString() !=
            right.events[index].SerializeAsString()) {
            return false;
        }
    }
    return true;
}

}  // namespace

int main() {
    using traceforge::generator::GeneratorConfig;
    using traceforge::generator::WorkloadGenerator;

    GeneratorConfig baseline_config;
    baseline_config.seed = 42;
    baseline_config.duration = std::chrono::seconds{1};
    const auto baseline = WorkloadGenerator{baseline_config}.generate();
    const auto counts = count_payloads(baseline);

    if (counts.accelerometer != 50 || counts.gyroscope != 50 ||
        counts.camera != 30 || counts.gps != 10 || counts.temperature != 1 ||
        counts.fault != 2 || counts.controller < 3 || counts.controller > 4) {
        std::cerr << "Generator did not produce the configured sensor rates\n";
        return 1;
    }
    if (baseline.stats.generated_events != baseline.events.size() ||
        baseline.stats.dropped_events != 0 ||
        baseline.stats.duplicated_events != 0 ||
        baseline.stats.reordered_pairs != 0) {
        std::cerr << "Unexpected baseline generation statistics\n";
        return 1;
    }

    std::map<std::string, traceforge::telemetry::StreamValidator> validators;
    for (const auto& event : baseline.events) {
        const auto validation = validators[event.producer_id()].accept(event);
        if (!validation.accepted) {
            std::cerr << "Generated event failed validation: "
                      << validation.error << '\n';
            return 1;
        }
    }

    GeneratorConfig injected_config = baseline_config;
    injected_config.seed = 8675309;
    injected_config.max_jitter = std::chrono::microseconds{200};
    injected_config.clock_drift_ppm = 125;
    injected_config.faults.drop_per_million = 100'000;
    injected_config.faults.duplicate_per_million = 200'000;
    injected_config.faults.reorder_per_million = 300'000;
    const auto injected_first = WorkloadGenerator{injected_config}.generate();
    const auto injected_second = WorkloadGenerator{injected_config}.generate();
    if (injected_first.hash != injected_second.hash ||
        !same_events(injected_first, injected_second)) {
        std::cerr << "Identical seeds produced different workloads\n";
        return 1;
    }

    ++injected_config.seed;
    const auto different_seed = WorkloadGenerator{injected_config}.generate();
    if (injected_first.hash == different_seed.hash) {
        std::cerr << "Different seeds produced the same workload hash\n";
        return 1;
    }

    GeneratorConfig duplicate_config = baseline_config;
    duplicate_config.faults.duplicate_per_million = 1'000'000;
    const auto duplicated = WorkloadGenerator{duplicate_config}.generate();
    if (duplicated.events.size() != baseline.events.size() * 2 ||
        duplicated.stats.duplicated_events != baseline.events.size()) {
        std::cerr << "Forced duplication did not duplicate every event\n";
        return 1;
    }

    GeneratorConfig drop_config = baseline_config;
    drop_config.faults.drop_per_million = 1'000'000;
    const auto dropped = WorkloadGenerator{drop_config}.generate();
    if (!dropped.events.empty() ||
        dropped.stats.dropped_events != baseline.events.size()) {
        std::cerr << "Forced drops did not remove every event\n";
        return 1;
    }

    GeneratorConfig reorder_config = baseline_config;
    reorder_config.faults.reorder_per_million = 1'000'000;
    const auto reordered = WorkloadGenerator{reorder_config}.generate();
    if (reordered.stats.reordered_pairs != reordered.events.size() / 2 ||
        reordered.hash == baseline.hash) {
        std::cerr << "Forced reordering did not swap every adjacent pair\n";
        return 1;
    }

    return 0;
}
