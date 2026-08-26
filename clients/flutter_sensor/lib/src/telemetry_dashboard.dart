import 'package:flutter/material.dart';

import 'sample_stats.dart';
import 'telemetry_controller.dart';
import 'telemetry_publisher.dart';
import 'telemetry_sample.dart';
import 'telemetry_source.dart';

class TelemetryDashboard extends StatefulWidget {
  const TelemetryDashboard({required this.source, this.publisher, super.key});

  final TelemetrySource source;
  final TelemetryPublisher? publisher;

  @override
  State<TelemetryDashboard> createState() => _TelemetryDashboardState();
}

class _TelemetryDashboardState extends State<TelemetryDashboard> {
  late final TelemetryController _controller;

  @override
  void initState() {
    super.initState();
    _controller = TelemetryController(
      source: widget.source,
      publisher: widget.publisher,
    );
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('TraceForge Sensor')),
      body: ListenableBuilder(
        listenable: _controller,
        builder: (context, _) {
          return ListView(
            padding: const EdgeInsets.all(16),
            children: [
              _StatusPanel(controller: _controller),
              const SizedBox(height: 8),
              Text(
                _controller.transportStatus,
                style: Theme.of(context).textTheme.bodySmall,
              ),
              const SizedBox(height: 12),
              _MotionPanel(
                title: 'Accelerometer',
                unit: 'm/s²',
                sample: _controller.accelerometer,
                stats: _controller.accelerometerStats,
              ),
              const SizedBox(height: 12),
              _MotionPanel(
                title: 'Gyroscope',
                unit: 'rad/s',
                sample: _controller.gyroscope,
                stats: _controller.gyroscopeStats,
              ),
              const SizedBox(height: 12),
              _LocationPanel(
                sample: _controller.location,
                stats: _controller.locationStats,
              ),
              const SizedBox(height: 16),
              Text(
                'Source timestamps come from each phone sensor API. Arrival '
                'timestamps use the phone wall clock. Neither is comparable '
                'to the future desktop collector clock without synchronization.',
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ],
          );
        },
      ),
    );
  }
}

class _StatusPanel extends StatelessWidget {
  const _StatusPanel({required this.controller});

  final TelemetryController controller;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Icon(
              controller.isRunning ? Icons.sensors : Icons.sensors_off,
              color: controller.isRunning
                  ? Theme.of(context).colorScheme.primary
                  : Theme.of(context).colorScheme.outline,
            ),
            const SizedBox(width: 12),
            Expanded(child: Text(controller.status)),
            FilledButton(
              onPressed: controller.isRunning
                  ? controller.stop
                  : controller.start,
              child: Text(controller.isRunning ? 'Stop' : 'Start'),
            ),
          ],
        ),
      ),
    );
  }
}

class _MotionPanel extends StatelessWidget {
  const _MotionPanel({
    required this.title,
    required this.unit,
    required this.sample,
    required this.stats,
  });

  final String title;
  final String unit;
  final MotionSample? sample;
  final SampleStats stats;

  @override
  Widget build(BuildContext context) {
    return _SensorCard(
      title: title,
      stats: stats,
      rows: [
        _ValueRow(label: 'x', value: _value(sample?.x, unit)),
        _ValueRow(label: 'y', value: _value(sample?.y, unit)),
        _ValueRow(label: 'z', value: _value(sample?.z, unit)),
        _ValueRow(
          label: 'Source timestamp',
          value: _timestamp(sample?.sourceTimestamp),
        ),
        _ValueRow(
          label: 'Phone arrival',
          value: sample?.receivedAt.toIso8601String() ?? 'Waiting',
        ),
      ],
    );
  }
}

class _LocationPanel extends StatelessWidget {
  const _LocationPanel({required this.sample, required this.stats});

  final LocationSample? sample;
  final SampleStats stats;

  @override
  Widget build(BuildContext context) {
    return _SensorCard(
      title: 'Location',
      stats: stats,
      rows: [
        _ValueRow(
          label: 'Latitude',
          value: sample?.latitude.toStringAsFixed(6) ?? 'Waiting',
        ),
        _ValueRow(
          label: 'Longitude',
          value: sample?.longitude.toStringAsFixed(6) ?? 'Waiting',
        ),
        _ValueRow(
          label: 'Accuracy',
          value: sample == null
              ? 'Waiting'
              : '${sample!.horizontalAccuracyMeters.toStringAsFixed(1)} m',
        ),
        _ValueRow(
          label: 'Source timestamp',
          value: _timestamp(sample?.sourceTimestamp),
        ),
        _ValueRow(
          label: 'Provider',
          value: sample == null
              ? 'Waiting'
              : sample!.isMocked
              ? 'Mocked by device'
              : 'Physical device',
        ),
      ],
    );
  }
}

class _SensorCard extends StatelessWidget {
  const _SensorCard({
    required this.title,
    required this.stats,
    required this.rows,
  });

  final String title;
  final SampleStats stats;
  final List<Widget> rows;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(
                  child: Text(
                    title,
                    style: Theme.of(context).textTheme.titleLarge,
                  ),
                ),
                Text(
                  '${stats.observedRateHz.toStringAsFixed(1)} Hz · '
                  '${stats.count} samples',
                ),
              ],
            ),
            const Divider(),
            ...rows,
            if (stats.nonMonotonicTimestamps > 0)
              _ValueRow(
                label: 'Timestamp warnings',
                value: '${stats.nonMonotonicTimestamps} non-monotonic',
              ),
          ],
        ),
      ),
    );
  }
}

class _ValueRow extends StatelessWidget {
  const _ValueRow({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        children: [
          Expanded(child: Text(label)),
          Flexible(
            child: Text(
              value,
              textAlign: TextAlign.end,
              style: const TextStyle(
                fontFeatures: [FontFeature.tabularFigures()],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

String _value(double? value, String unit) {
  return value == null ? 'Waiting' : '${value.toStringAsFixed(3)} $unit';
}

String _timestamp(DateTime? timestamp) {
  return timestamp == null
      ? 'Waiting'
      : '${timestamp.microsecondsSinceEpoch} µs (sensor clock)';
}
