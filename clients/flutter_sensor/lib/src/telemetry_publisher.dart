import 'telemetry_sample.dart';

class TelemetryUploadSummary {
  const TelemetryUploadSummary({
    required this.producerId,
    required this.acceptedEvents,
    required this.firstSequenceNumber,
    required this.lastSequenceNumber,
    required this.sequenceGaps,
  });

  final String producerId;
  final int acceptedEvents;
  final int firstSequenceNumber;
  final int lastSequenceNumber;
  final int sequenceGaps;
}

abstract interface class TelemetryPublisher {
  String get endpoint;

  Future<void> start();

  void publishMotion(MotionSample sample);

  void publishLocation(LocationSample sample);

  Future<TelemetryUploadSummary> stop();
}
