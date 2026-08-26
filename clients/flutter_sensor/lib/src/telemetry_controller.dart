import 'dart:async';

import 'package:flutter/foundation.dart';

import 'sample_stats.dart';
import 'telemetry_sample.dart';
import 'telemetry_source.dart';

class TelemetryController extends ChangeNotifier {
  TelemetryController({
    required this.source,
    this.refreshInterval = const Duration(milliseconds: 200),
  });

  final TelemetrySource source;
  final Duration refreshInterval;

  final SampleStats accelerometerStats = SampleStats();
  final SampleStats gyroscopeStats = SampleStats();
  final SampleStats locationStats = SampleStats();

  MotionSample? accelerometer;
  MotionSample? gyroscope;
  LocationSample? location;

  bool get isRunning => _isRunning;
  bool _isRunning = false;

  String get status => _status;
  String _status = 'Stopped';

  StreamSubscription<MotionSample>? _accelerometerSubscription;
  StreamSubscription<MotionSample>? _gyroscopeSubscription;
  StreamSubscription<LocationSample>? _locationSubscription;
  Timer? _refreshTimer;

  Future<void> start() async {
    if (_isRunning) {
      return;
    }

    _isRunning = true;
    _status = 'Starting motion sensors';
    notifyListeners();

    _accelerometerSubscription = source.accelerometerSamples().listen(
      _onAccelerometer,
      onError: (Object error) => _onStreamError('Accelerometer', error),
    );
    _gyroscopeSubscription = source.gyroscopeSamples().listen(
      _onGyroscope,
      onError: (Object error) => _onStreamError('Gyroscope', error),
    );

    _refreshTimer = Timer.periodic(refreshInterval, (_) => notifyListeners());

    try {
      final locationReadiness = await source.prepareLocation();
      if (!_isRunning) {
        return;
      }

      switch (locationReadiness) {
        case LocationReadiness.ready:
          _locationSubscription = source.locationSamples().listen(
            _onLocation,
            onError: (Object error) => _onStreamError('Location', error),
          );
          _status = 'Capturing motion and location';
        case LocationReadiness.servicesDisabled:
          _status = 'Capturing motion; location services are disabled';
        case LocationReadiness.permissionDenied:
          _status = 'Capturing motion; location permission was denied';
        case LocationReadiness.permissionDeniedForever:
          _status = 'Capturing motion; location permission is blocked';
      }
    } on Object catch (error) {
      _status = 'Capturing motion; location failed: $error';
    }

    notifyListeners();
  }

  Future<void> stop() async {
    if (!_isRunning) {
      return;
    }

    _isRunning = false;
    _refreshTimer?.cancel();
    _refreshTimer = null;

    await Future.wait<void>([
      _cancel(_accelerometerSubscription),
      _cancel(_gyroscopeSubscription),
      _cancel(_locationSubscription),
    ]);

    _accelerometerSubscription = null;
    _gyroscopeSubscription = null;
    _locationSubscription = null;
    _status = 'Stopped';
    notifyListeners();
  }

  void _onAccelerometer(MotionSample sample) {
    accelerometer = sample;
    accelerometerStats.add(sample.sourceTimestamp);
  }

  void _onGyroscope(MotionSample sample) {
    gyroscope = sample;
    gyroscopeStats.add(sample.sourceTimestamp);
  }

  void _onLocation(LocationSample sample) {
    location = sample;
    locationStats.add(sample.sourceTimestamp);
  }

  void _onStreamError(String streamName, Object error) {
    _status = '$streamName stream failed: $error';
    notifyListeners();
  }

  static Future<void> _cancel(StreamSubscription<dynamic>? subscription) async {
    await subscription?.cancel();
  }

  @override
  void dispose() {
    _refreshTimer?.cancel();
    unawaited(_accelerometerSubscription?.cancel());
    unawaited(_gyroscopeSubscription?.cancel());
    unawaited(_locationSubscription?.cancel());
    super.dispose();
  }
}
