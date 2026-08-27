import 'dart:async';

import 'package:grpc/grpc.dart' hide ClientChannel;
import 'package:grpc/grpc.dart' as grpc show ClientChannel;
import 'package:grpc/service_api.dart' show ClientChannel;

import '../generated/traceforge/v1/telemetry.pb.dart' as proto;
import '../generated/traceforge/v1/telemetry.pbgrpc.dart';
import 'telemetry_encoder.dart';
import 'telemetry_publisher.dart';
import 'telemetry_sample.dart';

typedef GrpcChannelFactory = ClientChannel Function();
typedef GrpcStreamStarter = Future<proto.StreamSummary> Function(
  ClientChannel channel,
  Stream<proto.TelemetryEvent> events,
);

class GrpcTelemetryPublisher implements TelemetryPublisher {
  GrpcTelemetryPublisher({
    required this.host,
    required this.port,
    required String producerId,
    this.connectionTimeout = const Duration(seconds: 5),
    this.stopTimeout = const Duration(seconds: 5),
    this.channelFactory,
    this.streamStarter,
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
  final Duration connectionTimeout;
  final Duration stopTimeout;
  final TelemetryEncoder _encoder;
  final GrpcChannelFactory? channelFactory;
  final GrpcStreamStarter? streamStarter;

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

    final channel =
        channelFactory?.call() ??
        grpc.ClientChannel(
          host,
          port: port,
          options: ChannelOptions(
            credentials: const ChannelCredentials.insecure(),
            connectTimeout: connectionTimeout,
          ),
        );
    final events = StreamController<proto.TelemetryEvent>();
    final ready = Completer<void>();
    final stateSubscription = channel.onConnectionStateChanged.listen(
      (state) {
        if (state == ConnectionState.ready && !ready.isCompleted) {
          ready.complete();
        }
      },
      onError: (Object error, StackTrace stackTrace) {
        if (!ready.isCompleted) {
          ready.completeError(error, stackTrace);
        }
      },
      onDone: () {
        if (!ready.isCompleted) {
          ready.completeError(
            StateError('gRPC channel closed before becoming ready'),
          );
        }
      },
    );
    final response = (streamStarter ?? _startTelemetryStream)(
      channel,
      events.stream,
    );
    final completion = response.then(
      _RpcCompletion.success,
      onError: (Object error, StackTrace stackTrace) {
        return _RpcCompletion.failure(error, stackTrace);
      },
    );

    try {
      await ready.future.timeout(
        connectionTimeout,
        onTimeout: () => throw TimeoutException(
          'Collector at $endpoint did not become ready within '
          '${connectionTimeout.inMilliseconds} ms',
          connectionTimeout,
        ),
      );
    } on Object {
      await stateSubscription.cancel();
      await _terminateIgnoringErrors(channel);
      await events.close();
      rethrow;
    }

    await stateSubscription.cancel();
    _channel = channel;
    _events = events;
    _nextSequenceNumber = 0;
    _completion = completion;
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

    late final _RpcCompletion result;
    try {
      result = await (() async {
        await events.close();
        return completion;
      })().timeout(stopTimeout);
    } on TimeoutException {
      await _terminateIgnoringErrors(channel);
      throw TimeoutException(
        'Collector at $endpoint did not acknowledge the stream within '
        '${stopTimeout.inMilliseconds} ms',
        stopTimeout,
      );
    }

    if (result.error case final error?) {
      await _terminateIgnoringErrors(channel);
      Error.throwWithStackTrace(error, result.stackTrace!);
    }

    try {
      await channel.shutdown().timeout(stopTimeout);
    } on TimeoutException {
      await _terminateIgnoringErrors(channel);
      throw TimeoutException(
        'gRPC channel for $endpoint did not shut down within '
        '${stopTimeout.inMilliseconds} ms',
        stopTimeout,
      );
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

  static Future<proto.StreamSummary> _startTelemetryStream(
    ClientChannel channel,
    Stream<proto.TelemetryEvent> events,
  ) {
    return TelemetryCollectorClient(channel).streamTelemetry(events);
  }

  static Future<void> _terminateIgnoringErrors(ClientChannel channel) async {
    try {
      await channel.terminate();
    } on Object {
      // Preserve the connection or RPC error that triggered cleanup.
    }
  }
}

class _RpcCompletion {
  const _RpcCompletion.success(this.summary) : error = null, stackTrace = null;

  const _RpcCompletion.failure(this.error, this.stackTrace) : summary = null;

  final proto.StreamSummary? summary;
  final Object? error;
  final StackTrace? stackTrace;
}
