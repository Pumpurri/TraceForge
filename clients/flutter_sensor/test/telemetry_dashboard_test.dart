import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:traceforge_sensor/main.dart';
import 'package:traceforge_sensor/src/telemetry_sample.dart';
import 'package:traceforge_sensor/src/telemetry_source.dart';

void main() {
  testWidgets('starts motion capture when location permission is denied', (
    tester,
  ) async {
    final source = DashboardTestSource();

    await tester.pumpWidget(TraceForgeSensorApp(source: source));
    expect(find.text('Stopped'), findsOneWidget);
    expect(find.text('Start'), findsOneWidget);

    await tester.tap(find.text('Start'));
    await tester.pump();

    expect(
      find.text('Capturing motion; location permission was denied'),
      findsOneWidget,
    );
    expect(find.text('Stop'), findsOneWidget);
    expect(find.textContaining('0.0 Hz'), findsAtLeastNWidgets(2));

    await tester.pumpWidget(const SizedBox.shrink());
    await source.close();
  });
}

class DashboardTestSource implements TelemetrySource {
  final _accelerometer = StreamController<MotionSample>.broadcast();
  final _gyroscope = StreamController<MotionSample>.broadcast();
  final _location = StreamController<LocationSample>.broadcast();

  @override
  Stream<MotionSample> accelerometerSamples() => _accelerometer.stream;

  @override
  Stream<MotionSample> gyroscopeSamples() => _gyroscope.stream;

  @override
  Stream<LocationSample> locationSamples() => _location.stream;

  @override
  Future<LocationReadiness> prepareLocation() async {
    return LocationReadiness.permissionDenied;
  }

  Future<void> close() async {
    await _accelerometer.close();
    await _gyroscope.close();
    await _location.close();
  }
}
