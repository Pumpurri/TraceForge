import 'dart:io';

import 'package:traceforge_sensor/src/grpc_telemetry_publisher.dart';
import 'package:traceforge_sensor/src/telemetry_sample.dart';

Future<void> main(List<String> arguments) async {
  final host = _option(arguments, '--host') ?? '127.0.0.1';
  final port = int.parse(_option(arguments, '--port') ?? '50051');
  final publisher = GrpcTelemetryPublisher(
    host: host,
    port: port,
    producerId: 'dart-transport-smoke-test',
  );
  final timestamp = DateTime.now().toUtc();

  await publisher.start();
  publisher.publishMotion(
    MotionSample(
      sensor: MotionSensor.accelerometer,
      x: 0.1,
      y: 0.2,
      z: 9.81,
      sourceTimestamp: timestamp,
      receivedAt: timestamp,
    ),
  );
  publisher.publishMotion(
    MotionSample(
      sensor: MotionSensor.gyroscope,
      x: 0.01,
      y: 0.02,
      z: 0.03,
      sourceTimestamp: timestamp.add(const Duration(milliseconds: 20)),
      receivedAt: timestamp.add(const Duration(milliseconds: 20)),
    ),
  );
  publisher.publishLocation(
    LocationSample(
      latitude: 29.6516,
      longitude: -82.3248,
      horizontalAccuracyMeters: 4.0,
      sourceTimestamp: timestamp.add(const Duration(milliseconds: 40)),
      receivedAt: timestamp.add(const Duration(milliseconds: 40)),
      isMocked: true,
    ),
  );

  final summary = await publisher.stop();
  stdout.writeln(
    'Collector accepted ${summary.acceptedEvents} events from '
    '${summary.producerId}; gaps=${summary.sequenceGaps}',
  );

  if (summary.acceptedEvents != 3 || summary.sequenceGaps != 0) {
    stderr.writeln('Unexpected collector summary');
    exitCode = 1;
  }
}

String? _option(List<String> arguments, String name) {
  final index = arguments.indexOf(name);
  if (index == -1) {
    return null;
  }
  if (index + 1 >= arguments.length) {
    throw FormatException('Missing value after $name');
  }
  return arguments[index + 1];
}
