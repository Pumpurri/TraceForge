import 'dart:async';

import 'package:grpc/grpc.dart';

import '../generated/traceforge/v1/telemetry.pb.dart' as proto;
import '../generated/traceforge/v1/telemetry.pbgrpc.dart';
import 'telemetry_encoder.dart';
import 'telemetry_publisher.dart';
import 'telemetry_sample.dart';

class GrpcTelemetryPublisher implements TelemetryPublisher {
  GrpcTelemetryPublisher({
    required this.host,
    required this.port,
    required String producerId,
  }) : _encoder = TelemetryEncoder(producerId: producerId);

  factory GrpcTelemetryPublisher.fromEnvironment() {
    return GrpcTelemetryPublisher(
      host: const String.fromEnvironment(
        'TRACEFORGE_COLLECTOR_HOST',
        defaultValue: '127.0.0.1',
      ),
      port: const int.fromEnvironment(
        'TRACEFORGE_COLLECTOR_PORT',
        defaultValue: 50051,
      ),
      producerId: const String.fromEnvironment(
        'TRACEFORGE_PRODUCER_ID',
        defaultValue: 'phone-sensor',
      ),
    );
  }

  final String host;
  final int port;
  final TelemetryEncoder _encoder;

  ClientChannel? _channel;
  StreamController<proto.TelemetryEvent>? _events;
  Future<_RpcCompletion>? _completion;
  int _nextSequenceNumber = 0;

  @override
  String get endpoint => '$host:$port';

  @override
  Future<void> start() async {
    if (_events != null) {
      return;
    }

    final channel = ClientChannel(
      host,
      port: port,
      options: const ChannelOptions(credentials: ChannelCredentials.insecure()),
    );
    final events = StreamController<proto.TelemetryEvent>();
    final response = TelemetryCollectorClient(channel)
        .streamTelemetry(events.stream);

    _channel = channel;
    _events = events;
    _nextSequenceNumber = 0;
    _completion = response.then(
      _RpcCompletion.success,
      onError: (Object error, StackTrace stackTrace) {
        return _RpcCompletion.failure(error, stackTrace);
      },
    );
  }

  @override
  void publishMotion(MotionSample sample) {
    _add(_encoder.encodeMotion(sample, _nextSequenceNumber++));
  }

  @override
  void publishLocation(LocationSample sample) {
    _add(_encoder.encodeLocation(sample, _nextSequenceNumber++));
  }

  void _add(proto.TelemetryEvent event) {
    final events = _events;
    if (events == null || events.isClosed) {
      throw StateError('Telemetry publisher is not running');
    }
    events.add(event);
  }

  @override
  Future<TelemetryUploadSummary> stop() async {
    final events = _events;
    final completion = _completion;
    final channel = _channel;
    if (events == null || completion == null || channel == null) {
      return const TelemetryUploadSummary(
        producerId: '',
        acceptedEvents: 0,
        firstSequenceNumber: 0,
        lastSequenceNumber: 0,
        sequenceGaps: 0,
      );
    }

    _events = null;
    _completion = null;
    _channel = null;

    await events.close();
    final result = await completion;
    await channel.shutdown();

    if (result.error case final error?) {
      Error.throwWithStackTrace(error, result.stackTrace!);
    }

    final summary = result.summary!;
    return TelemetryUploadSummary(
      producerId: summary.producerId,
      acceptedEvents: summary.acceptedEvents.toInt(),
      firstSequenceNumber: summary.firstSequenceNumber.toInt(),
      lastSequenceNumber: summary.lastSequenceNumber.toInt(),
      sequenceGaps: summary.sequenceGaps.toInt(),
    );
  }
}

class _RpcCompletion {
  const _RpcCompletion.success(this.summary) : error = null, stackTrace = null;

  const _RpcCompletion.failure(this.error, this.stackTrace) : summary = null;

  final proto.StreamSummary? summary;
  final Object? error;
  final StackTrace? stackTrace;
}
