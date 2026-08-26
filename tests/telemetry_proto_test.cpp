#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "traceforge/telemetry/validation.hpp"
#include "traceforge/v1/telemetry.pb.h"

namespace {

traceforge::v1::TelemetryEvent base_event(std::uint64_t sequence_number) {
    traceforge::v1::TelemetryEvent event;
    event.set_schema_version(traceforge::telemetry::kTelemetrySchemaVersion);
    event.set_producer_id("roundtrip-producer");
    event.set_sequence_number(sequence_number);
    event.set_source_timestamp_ns(1'777'000'000'000'000'000 +
                                  static_cast<std::int64_t>(sequence_number));
    event.set_source_clock(traceforge::v1::CLOCK_DOMAIN_UTC);
    return event;
}

}  // namespace

int main() {
    using Event = traceforge::v1::TelemetryEvent;

    std::vector<Event> events;

    auto accelerometer = base_event(1);
    accelerometer.mutable_accelerometer()->set_x(1.0);
    events.push_back(std::move(accelerometer));

    auto gyroscope = base_event(2);
    gyroscope.mutable_gyroscope()->set_z(0.5);
    events.push_back(std::move(gyroscope));

    auto gps = base_event(3);
    gps.mutable_gps()->set_latitude_degrees(29.6516);
    gps.mutable_gps()->set_longitude_degrees(-82.3248);
    gps.mutable_gps()->set_horizontal_accuracy_meters(3.2);
    events.push_back(std::move(gps));

    auto temperature = base_event(4);
    temperature.mutable_temperature()->set_degrees_celsius(42.5);
    events.push_back(std::move(temperature));

    auto camera = base_event(5);
    camera.mutable_camera()->set_frame_number(100);
    camera.mutable_camera()->set_width_pixels(1920);
    camera.mutable_camera()->set_height_pixels(1080);
    camera.mutable_camera()->set_exposure_time_ns(8'000'000);
    events.push_back(std::move(camera));

    auto controller = base_event(6);
    controller.mutable_controller_status()->set_state(
        traceforge::v1::ControllerStatus::STATE_READY);
    controller.mutable_controller_status()->set_detail("nominal");
    events.push_back(std::move(controller));

    auto fault = base_event(7);
    fault.mutable_fault()->set_severity(
        traceforge::v1::FaultEvent::SEVERITY_WARNING);
    fault.mutable_fault()->set_code(17);
    fault.mutable_fault()->set_message("temperature approaching limit");
    events.push_back(std::move(fault));

    const std::vector<Event::PayloadCase> expected_payloads{
        Event::kAccelerometer, Event::kGyroscope, Event::kGps,
        Event::kTemperature,   Event::kCamera,    Event::kControllerStatus,
        Event::kFault,
    };

    for (std::size_t index = 0; index < events.size(); ++index) {
        std::string encoded;
        if (!events[index].SerializeToString(&encoded)) {
            std::cerr << "Failed to serialize payload " << index << '\n';
            return 1;
        }

        Event decoded;
        if (!decoded.ParseFromString(encoded) ||
            decoded.payload_case() != expected_payloads[index] ||
            decoded.producer_id() != "roundtrip-producer" ||
            decoded.sequence_number() != index + 1) {
            std::cerr << "Round trip changed payload " << index << '\n';
            return 1;
        }
    }

    return 0;
}
