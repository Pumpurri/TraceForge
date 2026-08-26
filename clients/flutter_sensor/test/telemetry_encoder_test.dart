import 'package:flutter_test/flutter_test.dart';
import 'package:traceforge_sensor/generated/traceforge/v1/telemetry.pb.dart';
import 'package:traceforge_sensor/src/telemetry_encoder.dart';
import 'package:traceforge_sensor/src/telemetry_sample.dart';

void main() {
  const encoder = TelemetryEncoder(producerId: 'test-phone');
  final timestamp = DateTime.utc(2026, 8, 26, 12, 30);

  test('encodes accelerometer identity sequence timestamp and units', () {
    final event = encoder.encodeMotion(
      MotionSample(
        sensor: MotionSensor.accelerometer,
        x: 1.25,
        y: -2.5,
        z: 9.81,
        sourceTimestamp: timestamp,
        receivedAt: timestamp.add(const Duration(milliseconds: 1)),
      ),
      42,
    );

    expect(event.schemaVersion, TelemetryEncoder.schemaVersion);
    expect(event.producerId, 'test-phone');
    expect(event.sequenceNumber.toInt(), 42);
    expect(
      event.sourceTimestampNs.toInt(),
      timestamp.microsecondsSinceEpoch * 1000,
    );
    expect(event.sourceClock, ClockDomain.CLOCK_DOMAIN_UTC);
    expect(event.whichPayload(), TelemetryEvent_Payload.accelerometer);
    expect(event.accelerometer.x, 1.25);
    expect(event.accelerometer.y, -2.5);
    expect(event.accelerometer.z, 9.81);
  });

  test('encodes gyroscope as its distinct payload type', () {
    final event = encoder.encodeMotion(
      MotionSample(
        sensor: MotionSensor.gyroscope,
        x: 0.1,
        y: 0.2,
        z: 0.3,
        sourceTimestamp: timestamp,
        receivedAt: timestamp,
      ),
      43,
    );

    expect(event.whichPayload(), TelemetryEvent_Payload.gyroscope);
    expect(event.gyroscope.z, 0.3);
  });

  test('preserves GPS accuracy and mocked-provider marker', () {
    final event = encoder.encodeLocation(
      LocationSample(
        latitude: 29.6516,
        longitude: -82.3248,
        horizontalAccuracyMeters: 4.5,
        sourceTimestamp: timestamp,
        receivedAt: timestamp,
        isMocked: true,
      ),
      44,
    );

    expect(event.whichPayload(), TelemetryEvent_Payload.gps);
    expect(event.gps.latitudeDegrees, 29.6516);
    expect(event.gps.longitudeDegrees, -82.3248);
    expect(event.gps.horizontalAccuracyMeters, 4.5);
    expect(event.gps.isMocked, isTrue);
  });
}
