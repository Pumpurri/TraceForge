import 'package:geolocator/geolocator.dart';
import 'package:sensors_plus/sensors_plus.dart';

import 'telemetry_sample.dart';
import 'telemetry_source.dart';

class PluginTelemetrySource implements TelemetrySource {
  PluginTelemetrySource({DateTime Function()? clock})
    : _clock = clock ?? DateTime.now;

  final DateTime Function() _clock;

  @override
  Stream<MotionSample> accelerometerSamples() {
    return accelerometerEventStream(samplingPeriod: SensorInterval.gameInterval)
        .map(
          (event) => MotionSample(
            sensor: MotionSensor.accelerometer,
            x: event.x,
            y: event.y,
            z: event.z,
            sourceTimestamp: event.timestamp,
            receivedAt: _clock().toUtc(),
          ),
        );
  }

  @override
  Stream<MotionSample> gyroscopeSamples() {
    return gyroscopeEventStream(samplingPeriod: SensorInterval.gameInterval)
        .map(
          (event) => MotionSample(
            sensor: MotionSensor.gyroscope,
            x: event.x,
            y: event.y,
            z: event.z,
            sourceTimestamp: event.timestamp,
            receivedAt: _clock().toUtc(),
          ),
        );
  }

  @override
  Future<LocationReadiness> prepareLocation() async {
    if (!await Geolocator.isLocationServiceEnabled()) {
      return LocationReadiness.servicesDisabled;
    }

    var permission = await Geolocator.checkPermission();
    if (permission == LocationPermission.denied) {
      permission = await Geolocator.requestPermission();
    }

    return switch (permission) {
      LocationPermission.denied => LocationReadiness.permissionDenied,
      LocationPermission.deniedForever =>
        LocationReadiness.permissionDeniedForever,
      LocationPermission.whileInUse ||
      LocationPermission.always => LocationReadiness.ready,
      LocationPermission.unableToDetermine =>
        LocationReadiness.permissionDenied,
    };
  }

  @override
  Stream<LocationSample> locationSamples() {
    const settings = LocationSettings(
      accuracy: LocationAccuracy.high,
      distanceFilter: 1,
    );

    return Geolocator.getPositionStream(locationSettings: settings).map(
      (position) => LocationSample(
        latitude: position.latitude,
        longitude: position.longitude,
        horizontalAccuracyMeters: position.accuracy,
        sourceTimestamp: position.timestamp,
        receivedAt: _clock().toUtc(),
        isMocked: position.isMocked,
      ),
    );
  }
}
