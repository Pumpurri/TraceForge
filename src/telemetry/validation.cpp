#include "traceforge/telemetry/validation.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <string_view>

namespace traceforge::telemetry {
namespace {

constexpr std::size_t kMaxProducerIdBytes = 128;
constexpr std::size_t kMaxDetailBytes = 1'024;

[[nodiscard]] ValidationResult reject(std::string_view reason) {
    return {.accepted = false, .error = std::string{reason}};
}

[[nodiscard]] bool is_finite(const v1::Vector3& vector) {
    return std::isfinite(vector.x()) && std::isfinite(vector.y()) &&
           std::isfinite(vector.z());
}

[[nodiscard]] ValidationResult validate_payload(const v1::TelemetryEvent& event) {
    switch (event.payload_case()) {
        case v1::TelemetryEvent::kAccelerometer:
            return is_finite(event.accelerometer())
                       ? ValidationResult{.accepted = true, .error = {}}
                       : reject("accelerometer values must be finite");
        case v1::TelemetryEvent::kGyroscope:
            return is_finite(event.gyroscope())
                       ? ValidationResult{.accepted = true, .error = {}}
                       : reject("gyroscope values must be finite");
        case v1::TelemetryEvent::kGps: {
            const auto& gps = event.gps();
            if (!std::isfinite(gps.latitude_degrees()) ||
                !std::isfinite(gps.longitude_degrees()) ||
                !std::isfinite(gps.horizontal_accuracy_meters())) {
                return reject("GPS values must be finite");
            }
            if (gps.latitude_degrees() < -90.0 ||
                gps.latitude_degrees() > 90.0 ||
                gps.longitude_degrees() < -180.0 ||
                gps.longitude_degrees() > 180.0) {
                return reject("GPS coordinates are outside valid bounds");
            }
            if (gps.horizontal_accuracy_meters() < 0.0) {
                return reject("GPS accuracy cannot be negative");
            }
            return {.accepted = true, .error = {}};
        }
        case v1::TelemetryEvent::kTemperature:
            return std::isfinite(event.temperature().degrees_celsius())
                       ? ValidationResult{.accepted = true, .error = {}}
                       : reject("temperature must be finite");
        case v1::TelemetryEvent::kCamera:
            if (event.camera().width_pixels() == 0 ||
                event.camera().height_pixels() == 0) {
                return reject("camera dimensions must be non-zero");
            }
            return {.accepted = true, .error = {}};
        case v1::TelemetryEvent::kControllerStatus:
            if (!v1::ControllerStatus_State_IsValid(
                    static_cast<int>(event.controller_status().state())) ||
                event.controller_status().state() ==
                    v1::ControllerStatus::STATE_UNSPECIFIED) {
                return reject("controller state must be specified");
            }
            if (event.controller_status().detail().size() > kMaxDetailBytes) {
                return reject("controller detail is too long");
            }
            return {.accepted = true, .error = {}};
        case v1::TelemetryEvent::kFault:
            if (!v1::FaultEvent_Severity_IsValid(
                    static_cast<int>(event.fault().severity())) ||
                event.fault().severity() ==
                    v1::FaultEvent::SEVERITY_UNSPECIFIED) {
                return reject("fault severity must be specified");
            }
            if (event.fault().message().size() > kMaxDetailBytes) {
                return reject("fault message is too long");
            }
            return {.accepted = true, .error = {}};
        case v1::TelemetryEvent::PAYLOAD_NOT_SET:
            return reject("event payload is required");
    }

    return reject("event payload is unknown");
}

}  // namespace

ValidationResult StreamValidator::accept(const v1::TelemetryEvent& event) {
    if (event.schema_version() != kTelemetrySchemaVersion) {
        return reject("unsupported telemetry schema version");
    }
    if (event.producer_id().empty()) {
        return reject("producer_id is required");
    }
    if (event.producer_id().size() > kMaxProducerIdBytes) {
        return reject("producer_id is too long");
    }
    if (!v1::ClockDomain_IsValid(static_cast<int>(event.source_clock())) ||
        event.source_clock() == v1::CLOCK_DOMAIN_UNSPECIFIED) {
        return reject("source clock domain is required");
    }
    if (event.source_timestamp_ns() <= 0) {
        return reject("source timestamp must be positive");
    }
    if (!producer_id_.empty() && event.producer_id() != producer_id_) {
        return reject("producer_id cannot change within a stream");
    }
    if (accepted_events_ > 0 &&
        event.sequence_number() <= last_sequence_number_) {
        return reject("sequence numbers must increase within a stream");
    }

    auto payload_result = validate_payload(event);
    if (!payload_result.accepted) {
        return payload_result;
    }

    if (accepted_events_ == 0) {
        producer_id_ = event.producer_id();
        first_sequence_number_ = event.sequence_number();
    } else {
        sequence_gaps_ +=
            event.sequence_number() - last_sequence_number_ - 1;
    }

    last_sequence_number_ = event.sequence_number();
    ++accepted_events_;
    return {.accepted = true, .error = {}};
}

void StreamValidator::populate_summary(v1::StreamSummary& summary) const {
    summary.set_producer_id(producer_id_);
    summary.set_accepted_events(accepted_events_);
    summary.set_first_sequence_number(first_sequence_number_);
    summary.set_last_sequence_number(last_sequence_number_);
    summary.set_sequence_gaps(sequence_gaps_);
}

}  // namespace traceforge::telemetry
