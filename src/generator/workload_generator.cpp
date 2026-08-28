#include "traceforge/generator/workload_generator.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "traceforge/telemetry/validation.hpp"

namespace traceforge::generator {
namespace {

constexpr std::int64_t kNanosecondsPerSecond = 1'000'000'000;
constexpr std::uint32_t kProbabilityScale = 1'000'000;
constexpr std::int64_t kBaseTimestampNs = 1'700'000'000'000'000'000;
constexpr std::int64_t kMaxDurationNs = 3'600 * kNanosecondsPerSecond;
constexpr std::uint32_t kMaxSensorRateHz = 10'000;
constexpr std::uint64_t kMaxGeneratedEvents = 2'000'000;

class PortableRandom {
  public:
    explicit PortableRandom(std::uint64_t seed) : engine_(seed) {}

    [[nodiscard]] std::uint64_t bounded(std::uint64_t upper_exclusive) {
        if (upper_exclusive == 0) {
            return 0;
        }

        const auto threshold =
            static_cast<std::uint64_t>(-upper_exclusive) % upper_exclusive;
        while (true) {
            const auto value = engine_();
            if (value >= threshold) {
                return value % upper_exclusive;
            }
        }
    }

    [[nodiscard]] bool chance(std::uint32_t per_million) {
        return bounded(kProbabilityScale) < per_million;
    }

  private:
    std::mt19937_64 engine_;
};

void validate_probability(std::uint32_t probability, std::string_view name) {
    if (probability > kProbabilityScale) {
        throw std::invalid_argument(std::string{name} +
                                    " cannot exceed 1000000");
    }
}

void validate_config(const GeneratorConfig& config) {
    if (config.duration.count() <= 0 ||
        config.duration.count() > kMaxDurationNs) {
        throw std::invalid_argument(
            "duration must be between one nanosecond and one hour");
    }
    if (config.max_jitter.count() < 0 ||
        config.max_jitter.count() > kMaxDurationNs) {
        throw std::invalid_argument(
            "maximum jitter must be between zero and one hour");
    }
    if (config.clock_drift_ppm < -1'000'000 ||
        config.clock_drift_ppm > 1'000'000) {
        throw std::invalid_argument(
            "clock drift must be between -1000000 and 1000000 ppm");
    }
    validate_probability(config.faults.drop_per_million, "drop probability");
    validate_probability(config.faults.duplicate_per_million,
                         "duplicate probability");
    validate_probability(config.faults.reorder_per_million,
                         "reorder probability");

    const std::array rates{
        config.rates.accelerometer_hz, config.rates.gyroscope_hz,
        config.rates.camera_hz,        config.rates.gps_hz,
        config.rates.temperature_hz,   config.rates.fault_monitor_hz,
    };
    std::uint64_t combined_rate = 0;
    for (const auto rate : rates) {
        if (rate > kMaxSensorRateHz) {
            throw std::invalid_argument("sensor rate cannot exceed 10000 Hz");
        }
        combined_rate += rate;
    }
    const auto rounded_duration_seconds =
        static_cast<std::uint64_t>(config.duration.count() /
                                   kNanosecondsPerSecond) +
        1;
    if (combined_rate * rounded_duration_seconds > kMaxGeneratedEvents) {
        throw std::invalid_argument(
            "configuration would generate more than 2000000 events");
    }
}

[[nodiscard]] std::uint64_t event_count(std::int64_t duration_ns,
                                        std::uint32_t rate_hz) {
    if (rate_hz == 0) {
        return 0;
    }
    const auto whole_seconds =
        static_cast<std::uint64_t>(duration_ns / kNanosecondsPerSecond);
    const auto remaining_ns =
        static_cast<std::uint64_t>(duration_ns % kNanosecondsPerSecond);
    return whole_seconds * rate_hz +
           (remaining_ns * rate_hz) /
               static_cast<std::uint64_t>(kNanosecondsPerSecond);
}

[[nodiscard]] std::int64_t scheduled_time_ns(std::uint64_t index,
                                             std::uint32_t rate_hz) {
    const auto whole_seconds = index / rate_hz;
    const auto fractional_index = index % rate_hz;
    return static_cast<std::int64_t>(
        whole_seconds * static_cast<std::uint64_t>(kNanosecondsPerSecond) +
        (fractional_index * static_cast<std::uint64_t>(kNanosecondsPerSecond)) /
            rate_hz);
}

[[nodiscard]] std::int64_t apply_clock_drift(std::int64_t elapsed_ns,
                                             std::int64_t drift_ppm) {
    const auto whole_millions = elapsed_ns / kProbabilityScale;
    const auto remainder = elapsed_ns % kProbabilityScale;
    return elapsed_ns + whole_millions * drift_ppm +
           (remainder * drift_ppm) / kProbabilityScale;
}

[[nodiscard]] std::int64_t sample_jitter(PortableRandom& random,
                                         std::int64_t max_jitter_ns) {
    if (max_jitter_ns == 0) {
        return 0;
    }
    if (max_jitter_ns > (std::numeric_limits<std::int64_t>::max() - 1) / 2) {
        throw std::invalid_argument("maximum jitter is too large");
    }

    const auto width = static_cast<std::uint64_t>(max_jitter_ns * 2 + 1);
    return static_cast<std::int64_t>(random.bounded(width)) - max_jitter_ns;
}

[[nodiscard]] v1::TelemetryEvent base_event(std::string_view producer_id,
                                            std::uint64_t sequence_number,
                                            std::int64_t elapsed_ns,
                                            const GeneratorConfig& config,
                                            PortableRandom& random) {
    v1::TelemetryEvent event;
    event.set_schema_version(telemetry::kTelemetrySchemaVersion);
    event.set_producer_id(producer_id.data(), producer_id.size());
    event.set_sequence_number(sequence_number);
    event.set_source_timestamp_ns(
        kBaseTimestampNs +
        apply_clock_drift(elapsed_ns, config.clock_drift_ppm) +
        sample_jitter(random, config.max_jitter.count()));
    event.set_source_clock(v1::CLOCK_DOMAIN_UTC);
    return event;
}

template <typename PopulatePayload>
void append_periodic(std::vector<v1::TelemetryEvent>& events,
                     std::string_view producer_id, std::uint32_t rate_hz,
                     const GeneratorConfig& config, PortableRandom& random,
                     PopulatePayload populate_payload) {
    const auto count = event_count(config.duration.count(), rate_hz);
    for (std::uint64_t sequence = 0; sequence < count; ++sequence) {
        auto event =
            base_event(producer_id, sequence,
                       scheduled_time_ns(sequence, rate_hz), config, random);
        populate_payload(event, sequence, random);
        events.push_back(std::move(event));
    }
}

void append_controller_events(std::vector<v1::TelemetryEvent>& events,
                              const GeneratorConfig& config,
                              PortableRandom& random) {
    const auto duration_ns = config.duration.count();
    std::vector<std::pair<std::int64_t, v1::ControllerStatus::State>> states;
    states.emplace_back(0, v1::ControllerStatus::STATE_INITIALIZING);
    states.emplace_back(
        duration_ns / 4 +
            static_cast<std::int64_t>(random.bounded(static_cast<std::uint64_t>(
                std::max<std::int64_t>(duration_ns / 8, 1)))),
        v1::ControllerStatus::STATE_READY);
    if (random.chance(500'000)) {
        states.emplace_back(
            duration_ns * 5 / 8 +
                static_cast<std::int64_t>(
                    random.bounded(static_cast<std::uint64_t>(
                        std::max<std::int64_t>(duration_ns / 8, 1)))),
            v1::ControllerStatus::STATE_DEGRADED);
    }
    states.emplace_back(duration_ns - 1, v1::ControllerStatus::STATE_STOPPED);

    std::uint64_t sequence = 0;
    for (const auto& [elapsed_ns, state] : states) {
        auto event =
            base_event("sim-controller", sequence, elapsed_ns, config, random);
        event.mutable_controller_status()->set_state(state);
        event.mutable_controller_status()->set_detail(
            v1::ControllerStatus_State_Name(state));
        events.push_back(std::move(event));
        ++sequence;
    }
}

[[nodiscard]] std::string fault_message(v1::FaultEvent::Severity severity) {
    switch (severity) {
    case v1::FaultEvent::SEVERITY_WARNING:
        return "synthetic threshold warning";
    case v1::FaultEvent::SEVERITY_ERROR:
        return "synthetic sensor error";
    case v1::FaultEvent::SEVERITY_CRITICAL:
        return "synthetic critical fault";
    case v1::FaultEvent::SEVERITY_UNSPECIFIED:
        break;
    default:
        break;
    }
    return "synthetic unspecified fault";
}

void hash_byte(std::uint64_t& hash, std::uint8_t byte) {
    constexpr std::uint64_t kFnvPrime = 1'099'511'628'211;
    hash ^= byte;
    hash *= kFnvPrime;
}

}  // namespace

WorkloadGenerator::WorkloadGenerator(GeneratorConfig config)
    : config_(std::move(config)) {
    validate_config(config_);
}

GeneratedWorkload WorkloadGenerator::generate() const {
    PortableRandom random{config_.seed};
    std::vector<v1::TelemetryEvent> base_events;

    append_periodic(
        base_events, "sim-accelerometer", config_.rates.accelerometer_hz,
        config_, random,
        [](v1::TelemetryEvent& event, std::uint64_t sequence, PortableRandom&) {
            auto* vector = event.mutable_accelerometer();
            vector->set_x(static_cast<double>(sequence % 32) / 16.0 - 1.0);
            vector->set_y(static_cast<double>((sequence + 8) % 32) / 16.0 -
                          1.0);
            vector->set_z(9.8125 + static_cast<double>(sequence % 8) / 128.0);
        });
    append_periodic(
        base_events, "sim-gyroscope", config_.rates.gyroscope_hz, config_,
        random,
        [](v1::TelemetryEvent& event, std::uint64_t sequence, PortableRandom&) {
            auto* vector = event.mutable_gyroscope();
            vector->set_x(static_cast<double>(sequence % 16) / 256.0);
            vector->set_y(-static_cast<double>(sequence % 8) / 256.0);
            vector->set_z(static_cast<double>(sequence % 4) / 128.0);
        });
    append_periodic(
        base_events, "sim-camera", config_.rates.camera_hz, config_, random,
        [](v1::TelemetryEvent& event, std::uint64_t sequence, PortableRandom&) {
            auto* camera = event.mutable_camera();
            camera->set_frame_number(sequence);
            camera->set_width_pixels(1920);
            camera->set_height_pixels(1080);
            camera->set_exposure_time_ns(8'000'000 + (sequence % 4) * 250'000);
        });
    append_periodic(
        base_events, "sim-gps", config_.rates.gps_hz, config_, random,
        [](v1::TelemetryEvent& event, std::uint64_t sequence, PortableRandom&) {
            auto* gps = event.mutable_gps();
            gps->set_latitude_degrees(29.625 +
                                      static_cast<double>(sequence) / 65'536.0);
            gps->set_longitude_degrees(-82.375 + static_cast<double>(sequence) /
                                                     65'536.0);
            gps->set_horizontal_accuracy_meters(
                3.0 + static_cast<double>(sequence % 4) / 4.0);
            gps->set_is_mocked(false);
        });
    append_periodic(
        base_events, "sim-temperature", config_.rates.temperature_hz, config_,
        random,
        [](v1::TelemetryEvent& event, std::uint64_t sequence, PortableRandom&) {
            event.mutable_temperature()->set_degrees_celsius(
                40.0 + static_cast<double>(sequence % 16) / 16.0);
        });
    append_periodic(
        base_events, "sim-fault", config_.rates.fault_monitor_hz, config_,
        random,
        [](v1::TelemetryEvent& event, std::uint64_t sequence,
           PortableRandom& event_random) {
            const auto roll = event_random.bounded(100);
            const auto severity = roll < 70 ? v1::FaultEvent::SEVERITY_WARNING
                                  : roll < 95
                                      ? v1::FaultEvent::SEVERITY_ERROR
                                      : v1::FaultEvent::SEVERITY_CRITICAL;
            auto* fault = event.mutable_fault();
            fault->set_severity(severity);
            fault->set_code(static_cast<std::uint32_t>(1'000 + sequence));
            fault->set_message(fault_message(severity));
        });
    append_controller_events(base_events, config_, random);

    std::sort(
        base_events.begin(), base_events.end(),
        [](const v1::TelemetryEvent& left, const v1::TelemetryEvent& right) {
            if (left.source_timestamp_ns() != right.source_timestamp_ns()) {
                return left.source_timestamp_ns() < right.source_timestamp_ns();
            }
            if (left.producer_id() != right.producer_id()) {
                return left.producer_id() < right.producer_id();
            }
            return left.sequence_number() < right.sequence_number();
        });

    GeneratedWorkload result;
    result.stats.generated_events = base_events.size();
    result.events.reserve(base_events.size());
    for (const auto& event : base_events) {
        if (random.chance(config_.faults.drop_per_million)) {
            ++result.stats.dropped_events;
            continue;
        }
        result.events.push_back(event);
        if (random.chance(config_.faults.duplicate_per_million)) {
            result.events.push_back(event);
            ++result.stats.duplicated_events;
        }
    }

    for (std::size_t index = 0; index + 1 < result.events.size(); ++index) {
        if (random.chance(config_.faults.reorder_per_million)) {
            std::swap(result.events[index], result.events[index + 1]);
            ++result.stats.reordered_pairs;
            ++index;
        }
    }

    result.hash = stable_event_hash(result.events);
    return result;
}

std::uint64_t stable_event_hash(std::span<const v1::TelemetryEvent> events) {
    constexpr std::uint64_t kFnvOffsetBasis = 14'695'981'039'346'656'037ULL;
    std::uint64_t hash = kFnvOffsetBasis;
    for (const auto& event : events) {
        std::string encoded;
        if (!event.SerializeToString(&encoded)) {
            throw std::runtime_error("failed to serialize generated event");
        }

        auto remaining_size = static_cast<std::uint64_t>(encoded.size());
        for (int byte = 0; byte < 8; ++byte) {
            hash_byte(hash, static_cast<std::uint8_t>(remaining_size & 0xFFU));
            remaining_size >>= 8U;
        }
        for (const char value : encoded) {
            hash_byte(hash, static_cast<std::uint8_t>(
                                static_cast<unsigned char>(value)));
        }
    }
    return hash;
}

std::string format_hash(std::uint64_t hash) {
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(16) << hash;
    return output.str();
}

std::string_view
payload_name(v1::TelemetryEvent::PayloadCase payload) noexcept {
    switch (payload) {
    case v1::TelemetryEvent::kAccelerometer:
        return "accelerometer";
    case v1::TelemetryEvent::kGyroscope:
        return "gyroscope";
    case v1::TelemetryEvent::kGps:
        return "gps";
    case v1::TelemetryEvent::kTemperature:
        return "temperature";
    case v1::TelemetryEvent::kCamera:
        return "camera";
    case v1::TelemetryEvent::kControllerStatus:
        return "controller_status";
    case v1::TelemetryEvent::kFault:
        return "fault";
    case v1::TelemetryEvent::PAYLOAD_NOT_SET:
        return "unset";
    }
    return "unknown";
}

}  // namespace traceforge::generator
