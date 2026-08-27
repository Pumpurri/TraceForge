import 'dart:async';

import 'package:fixnum/fixnum.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart' hide ClientChannel;
import 'package:grpc/service_api.dart' show ClientChannel;
import 'package:traceforge_sensor/generated/traceforge/v1/telemetry.pb.dart'
    as proto;
import 'package:traceforge_sensor/src/grpc_telemetry_publisher.dart';

void main() {
  test('start waits until the gRPC channel is ready', () async {
    final channel = FakeClientChannel();
    final publisher = GrpcTelemetryPublisher(
      host: 'collector.test',
      port: 50051,
      producerId: 'test-phone',
      channelFactory: () => channel,
      streamStarter: successfulStream,
    );
    var started = false;

    final start = publisher.start().then((_) => started = true);
    await Future<void>.delayed(Duration.zero);
    expect(started, isFalse);

    channel.states.add(ConnectionState.connecting);
    await Future<void>.delayed(Duration.zero);
    expect(started, isFalse);

    channel.states.add(ConnectionState.ready);
    await start;
    expect(started, isTrue);

    final summary = await publisher.stop();
    expect(summary.acceptedEvents, 2);
    expect(channel.shutdownCalled, isTrue);
    expect(channel.terminateCalled, isFalse);
    await channel.close();
  });

  test('start times out and terminates an unreachable channel', () async {
    final channel = FakeClientChannel();
    final publisher = GrpcTelemetryPublisher(
      host: 'collector.test',
      port: 50051,
      producerId: 'test-phone',
      connectionTimeout: const Duration(milliseconds: 20),
      channelFactory: () => channel,
      streamStarter: neverCompletingStream,
    );

    await expectLater(
      publisher.start(),
      throwsA(
        isA<TimeoutException>().having(
          (error) => error.message,
          'message',
          contains('did not become ready'),
        ),
      ),
    );
    expect(channel.terminateCalled, isTrue);
    await channel.close();
  });

  test(
    'stop times out when the collector does not acknowledge the stream',
    () async {
      final channel = FakeClientChannel();
      final publisher = GrpcTelemetryPublisher(
        host: 'collector.test',
        port: 50051,
        producerId: 'test-phone',
        stopTimeout: const Duration(milliseconds: 20),
        channelFactory: () => channel,
        streamStarter: neverCompletingStream,
      );

      final start = publisher.start();
      channel.states.add(ConnectionState.ready);
      await start;

      await expectLater(
        publisher.stop(),
        throwsA(
          isA<TimeoutException>().having(
            (error) => error.message,
            'message',
            contains('did not acknowledge'),
          ),
        ),
      );
      expect(channel.terminateCalled, isTrue);
      expect(channel.shutdownCalled, isFalse);
      await channel.close();
    },
  );
}

Future<proto.StreamSummary> successfulStream(
  ClientChannel channel,
  Stream<proto.TelemetryEvent> events,
) {
  final response = Completer<proto.StreamSummary>();
  events.listen(
    (_) {},
    onDone: () {
      response.complete(
        proto.StreamSummary(
          producerId: 'test-phone',
          acceptedEvents: Int64(2),
          firstSequenceNumber: Int64.ZERO,
          lastSequenceNumber: Int64.ONE,
          sequenceGaps: Int64.ZERO,
        ),
      );
    },
  );
  return response.future;
}

Future<proto.StreamSummary> neverCompletingStream(
  ClientChannel channel,
  Stream<proto.TelemetryEvent> events,
) {
  events.listen((_) {});
  return Completer<proto.StreamSummary>().future;
}

class FakeClientChannel implements ClientChannel {
  final states = StreamController<ConnectionState>.broadcast(sync: true);
  bool shutdownCalled = false;
  bool terminateCalled = false;

  @override
  Stream<ConnectionState> get onConnectionStateChanged => states.stream;

  @override
  ClientCall<Q, R> createCall<Q, R>(
    ClientMethod<Q, R> method,
    Stream<Q> requests,
    CallOptions options,
  ) {
    throw UnsupportedError('The injected stream starter bypasses createCall');
  }

  @override
  Future<void> shutdown() async {
    shutdownCalled = true;
  }

  @override
  Future<void> terminate() async {
    terminateCalled = true;
  }

  Future<void> close() => states.close();
}
