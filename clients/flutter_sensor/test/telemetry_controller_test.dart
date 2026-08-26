import 'dart:async';

import 'package:flutter_test/flutter_test.dart';
import 'package:traceforge_sensor/src/telemetry_controller.dart';
import 'package:traceforge_sensor/src/telemetry_publisher.dart';
import 'package:traceforge_sensor/src/telemetry_sample.dart';
import 'package:traceforge_sensor/src/telemetry_source.dart';

void main() {
  test('captures motion and location samples from the source', () async {
    final source = FakeTelemetrySource();
    final controller = TelemetryController(source: source);
    final timestamp = DateTime.utc(2026, 8, 26);

    await controller.start();
    source.accelerometer.add(
      MotionSample(
        sensor: MotionSensor.accelerometer,
        x: 1,
        y: 2,
        z: 3,
        sourceTimestamp: timestamp,
        receivedAt: timestamp,
      ),
    );
    source.gyroscope.add(
      MotionSample(
        sensor: MotionSensor.gyroscope,
        x: 4,
        y: 5,
        z: 6,
        sourceTimestamp: timestamp,
        receivedAt: timestamp,
      ),
    );
    source.location.add(
      LocationSample(
        latitude: 29.6516,
        longitude: -82.3248,
        horizontalAccuracyMeters: 4,
        sourceTimestamp: timestamp,
        receivedAt: timestamp,
        isMocked: false,
      ),
    );
    await Future<void>.delayed(Duration.zero);

    expect(controller.accelerometer?.x, 1);
    expect(controller.gyroscope?.z, 6);
    expect(controller.location?.latitude, 29.6516);
    expect(controller.accelerometerStats.count, 1);
    expect(controller.gyroscopeStats.count, 1);
    expect(controller.locationStats.count, 1);
    expect(controller.status, 'Capturing motion and location');

    await controller.stop();
    controller.dispose();
    await source.close();
  });

  test('continues motion capture when location is denied', () async {
    final source = FakeTelemetrySource(
      readiness: LocationReadiness.permissionDenied,
    );
    final controller = TelemetryController(source: source);

    await controller.start();

    expect(controller.isRunning, isTrue);
    expect(controller.status, contains('location permission was denied'));
    expect(source.location.hasListener, isFalse);

    await controller.stop();
    controller.dispose();
    await source.close();
  });

  test('streams captured samples and reports the collector summary', () async {
    final source = FakeTelemetrySource();
    final publisher = FakeTelemetryPublisher();
    final controller = TelemetryController(
      source: source,
      publisher: publisher,
    );
    final timestamp = DateTime.utc(2026, 8, 26);

    await controller.start();
    source.accelerometer.add(
      MotionSample(
        sensor: MotionSensor.accelerometer,
        x: 1,
        y: 2,
        z: 3,
        sourceTimestamp: timestamp,
        receivedAt: timestamp,
      ),
    );
    source.location.add(
      LocationSample(
        latitude: 29.6516,
        longitude: -82.3248,
        horizontalAccuracyMeters: 4,
        sourceTimestamp: timestamp,
        receivedAt: timestamp,
        isMocked: false,
      ),
    );
    await Future<void>.delayed(Duration.zero);
    await controller.stop();

    expect(publisher.startCount, 1);
    expect(publisher.motionSamples, hasLength(1));
    expect(publisher.locationSamples, hasLength(1));
    expect(publisher.stopCount, 1);
    expect(controller.transportStatus, contains('accepted 2 events'));
    expect(controller.transportStatus, contains('0 sequence gaps'));

    controller.dispose();
    await source.close();
  });
}

class FakeTelemetrySource implements TelemetrySource {
  FakeTelemetrySource({this.readiness = LocationReadiness.ready});

  final LocationReadiness readiness;
  final accelerometer = StreamController<MotionSample>.broadcast();
  final gyroscope = StreamController<MotionSample>.broadcast();
  final location = StreamController<LocationSample>.broadcast();

  @override
  Stream<MotionSample> accelerometerSamples() => accelerometer.stream;

  @override
  Stream<MotionSample> gyroscopeSamples() => gyroscope.stream;

  @override
  Stream<LocationSample> locationSamples() => location.stream;

  @override
  Future<LocationReadiness> prepareLocation() async => readiness;

  Future<void> close() async {
    await accelerometer.close();
    await gyroscope.close();
    await location.close();
  }
}

class FakeTelemetryPublisher implements TelemetryPublisher {
  int startCount = 0;
  int stopCount = 0;
  final motionSamples = <MotionSample>[];
  final locationSamples = <LocationSample>[];

  @override
  String get endpoint => 'collector.test:50051';

  @override
  Future<void> start() async {
    startCount += 1;
  }

  @override
  void publishLocation(LocationSample sample) {
    locationSamples.add(sample);
  }

  @override
  void publishMotion(MotionSample sample) {
    motionSamples.add(sample);
  }

  @override
  Future<TelemetryUploadSummary> stop() async {
    stopCount += 1;
    return const TelemetryUploadSummary(
      producerId: 'test-phone',
      acceptedEvents: 2,
      firstSequenceNumber: 0,
      lastSequenceNumber: 1,
      sequenceGaps: 0,
    );
  }
}
