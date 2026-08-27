import 'dart:async';

import 'package:flutter/foundation.dart';

import 'sample_stats.dart';
import 'telemetry_publisher.dart';
import 'telemetry_sample.dart';
import 'telemetry_source.dart';

class TelemetryController extends ChangeNotifier {
  TelemetryController({
    required this.source,
    this.publisher,
    this.refreshInterval = const Duration(milliseconds: 200),
  });

  final TelemetrySource source;
  final TelemetryPublisher? publisher;
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

  String get transportStatus => _transportStatus;
  String _transportStatus = 'Network stream stopped';
  bool _publisherStarted = false;

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
    if (publisher case final publisher?) {
      _transportStatus = 'Connecting to ${publisher.endpoint}';
      notifyListeners();
      try {
        await publisher.start();
        _publisherStarted = true;
        _transportStatus = 'Streaming to ${publisher.endpoint}';
      } on Object catch (error) {
        _publisherStarted = false;
        _transportStatus = 'Network stream unavailable: $error';
      }
    } else {
      _transportStatus = 'Network stream disabled';
    }
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

    if (publisher case final publisher? when _publisherStarted) {
      _publisherStarted = false;
      try {
        final summary = await publisher.stop();
        _transportStatus =
            'Collector accepted ${summary.acceptedEvents} events; '
            '${summary.sequenceGaps} sequence gaps';
      } on Object catch (error) {
        _transportStatus = 'Network stream failed: $error';
      }
    }
    notifyListeners();
  }

  void _onAccelerometer(MotionSample sample) {
    accelerometer = sample;
    accelerometerStats.add(sample.sourceTimestamp);
    _publish(() => publisher?.publishMotion(sample));
  }

  void _onGyroscope(MotionSample sample) {
    gyroscope = sample;
    gyroscopeStats.add(sample.sourceTimestamp);
    _publish(() => publisher?.publishMotion(sample));
  }

  void _onLocation(LocationSample sample) {
    location = sample;
    locationStats.add(sample.sourceTimestamp);
    _publish(() => publisher?.publishLocation(sample));
  }

  void _publish(void Function() publish) {
    try {
      publish();
    } on Object catch (error) {
      _transportStatus = 'Network stream failed: $error';
    }
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
    if (publisher case final publisher? when _publisherStarted) {
      _publisherStarted = false;
      unawaited(_stopPublisherIgnoringErrors(publisher));
    }
    super.dispose();
  }

  static Future<void> _stopPublisherIgnoringErrors(
    TelemetryPublisher publisher,
  ) async {
    try {
      await publisher.stop();
    } on Object {
      // Disposal cannot surface an asynchronous transport error to the UI.
    }
  }
}
