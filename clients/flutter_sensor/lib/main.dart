import 'package:flutter/material.dart';

import 'src/grpc_telemetry_publisher.dart';
import 'src/plugin_telemetry_source.dart';
import 'src/telemetry_dashboard.dart';
import 'src/telemetry_publisher.dart';
import 'src/telemetry_source.dart';

void main() {
  runApp(
    TraceForgeSensorApp(
      source: PluginTelemetrySource(),
      publisher: GrpcTelemetryPublisher.fromEnvironment(),
    ),
  );
}

class TraceForgeSensorApp extends StatelessWidget {
  const TraceForgeSensorApp({required this.source, this.publisher, super.key});

  final TelemetrySource source;
  final TelemetryPublisher? publisher;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'TraceForge Sensor',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF0B7285),
          brightness: Brightness.dark,
        ),
        useMaterial3: true,
      ),
      home: TelemetryDashboard(source: source, publisher: publisher),
    );
  }
}
