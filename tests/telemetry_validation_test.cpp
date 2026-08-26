#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

#include "traceforge/telemetry/validation.hpp"
#include "traceforge/v1/telemetry.pb.h"

namespace {

traceforge::v1::TelemetryEvent make_accelerometer_event(
    std::uint64_t sequence_number) {
    traceforge::v1::TelemetryEvent event;
    event.set_schema_version(traceforge::telemetry::kTelemetrySchemaVersion);
    event.set_producer_id("phone-01");
    event.set_sequence_number(sequence_number);
    event.set_source_timestamp_ns(1'777'000'000'000'000'000);
    event.set_source_clock(traceforge::v1::CLOCK_DOMAIN_UTC);
    event.mutable_accelerometer()->set_x(1.0);
    event.mutable_accelerometer()->set_y(2.0);
    event.mutable_accelerometer()->set_z(3.0);
    return event;
}

bool expect_rejected(const traceforge::telemetry::ValidationResult& result,
                     std::string_view expected_error) {
    if (result.accepted || result.error.find(expected_error) == std::string::npos) {
        std::cerr << "Expected rejection containing: " << expected_error << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    using traceforge::telemetry::StreamValidator;

    StreamValidator valid_stream;
    if (!valid_stream.accept(make_accelerometer_event(10)).accepted ||
        !valid_stream.accept(make_accelerometer_event(13)).accepted) {
        std::cerr << "Valid events were rejected\n";
        return 1;
    }

    traceforge::v1::StreamSummary summary;
    valid_stream.populate_summary(summary);
    if (summary.producer_id() != "phone-01" ||
        summary.accepted_events() != 2 ||
        summary.first_sequence_number() != 10 ||
        summary.last_sequence_number() != 13 || summary.sequence_gaps() != 2) {
        std::cerr << "Unexpected stream summary\n";
        return 1;
    }

    StreamValidator duplicate_stream;
    static_cast<void>(duplicate_stream.accept(make_accelerometer_event(2)));
    if (!expect_rejected(duplicate_stream.accept(make_accelerometer_event(2)),
                         "sequence numbers")) {
        return 1;
    }

    StreamValidator invalid_payload_stream;
    auto invalid_payload = make_accelerometer_event(1);
    invalid_payload.mutable_accelerometer()->set_x(
        std::numeric_limits<double>::quiet_NaN());
    if (!expect_rejected(invalid_payload_stream.accept(invalid_payload),
                         "must be finite")) {
        return 1;
    }

    StreamValidator changed_producer_stream;
    static_cast<void>(
        changed_producer_stream.accept(make_accelerometer_event(1)));
    auto changed_producer = make_accelerometer_event(2);
    changed_producer.set_producer_id("phone-02");
    if (!expect_rejected(changed_producer_stream.accept(changed_producer),
                         "cannot change")) {
        return 1;
    }

    return 0;
}
