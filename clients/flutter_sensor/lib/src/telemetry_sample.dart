enum MotionSensor { accelerometer, gyroscope }

class MotionSample {
  const MotionSample({
    required this.sensor,
    required this.x,
    required this.y,
    required this.z,
    required this.sourceTimestamp,
    required this.receivedAt,
  });

  final MotionSensor sensor;
  final double x;
  final double y;
  final double z;

  /// Timestamp supplied by the phone sensor API.
  final DateTime sourceTimestamp;

  /// Wall-clock timestamp captured by the phone adapter.
  final DateTime receivedAt;
}

class LocationSample {
  const LocationSample({
    required this.latitude,
    required this.longitude,
    required this.horizontalAccuracyMeters,
    required this.sourceTimestamp,
    required this.receivedAt,
    required this.isMocked,
  });

  final double latitude;
  final double longitude;
  final double horizontalAccuracyMeters;
  final DateTime sourceTimestamp;
  final DateTime receivedAt;
  final bool isMocked;
}
