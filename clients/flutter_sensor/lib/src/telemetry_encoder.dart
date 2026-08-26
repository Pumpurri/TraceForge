import 'package:fixnum/fixnum.dart';

import '../generated/traceforge/v1/telemetry.pb.dart' as proto;
import 'telemetry_sample.dart';

class TelemetryEncoder {
  const TelemetryEncoder({required this.producerId});

  static const schemaVersion = 1;

  final String producerId;

  proto.TelemetryEvent encodeMotion(MotionSample sample, int sequenceNumber) {
    final event = _baseEvent(sample.sourceTimestamp, sequenceNumber);
    final vector = proto.Vector3(x: sample.x, y: sample.y, z: sample.z);

    switch (sample.sensor) {
      case MotionSensor.accelerometer:
        event.accelerometer = vector;
      case MotionSensor.gyroscope:
        event.gyroscope = vector;
    }

    return event;
  }

  proto.TelemetryEvent encodeLocation(
    LocationSample sample,
    int sequenceNumber,
  ) {
    return _baseEvent(sample.sourceTimestamp, sequenceNumber)
      ..gps = proto.GpsFix(
        latitudeDegrees: sample.latitude,
        longitudeDegrees: sample.longitude,
        horizontalAccuracyMeters: sample.horizontalAccuracyMeters,
        isMocked: sample.isMocked,
      );
  }

  proto.TelemetryEvent _baseEvent(
    DateTime sourceTimestamp,
    int sequenceNumber,
  ) {
    return proto.TelemetryEvent(
      schemaVersion: schemaVersion,
      producerId: producerId,
      sequenceNumber: Int64(sequenceNumber),
      sourceTimestampNs:
          Int64(sourceTimestamp.toUtc().microsecondsSinceEpoch) * Int64(1000),
      sourceClock: proto.ClockDomain.CLOCK_DOMAIN_UTC,
    );
  }
}
