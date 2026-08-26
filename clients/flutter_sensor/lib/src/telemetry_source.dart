import 'telemetry_sample.dart';

enum LocationReadiness {
  ready,
  servicesDisabled,
  permissionDenied,
  permissionDeniedForever,
}

abstract interface class TelemetrySource {
  Stream<MotionSample> accelerometerSamples();

  Stream<MotionSample> gyroscopeSamples();

  Future<LocationReadiness> prepareLocation();

  Stream<LocationSample> locationSamples();
}
