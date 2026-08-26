// This is a generated file - do not edit.
//
// Generated from traceforge/v1/telemetry.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:async' as $async;
import 'dart:core' as $core;

import 'package:grpc/service_api.dart' as $grpc;
import 'package:protobuf/protobuf.dart' as $pb;

import 'telemetry.pb.dart' as $0;

export 'telemetry.pb.dart';

@$pb.GrpcServiceName('traceforge.v1.TelemetryCollector')
class TelemetryCollectorClient extends $grpc.Client {
  /// The hostname for this service.
  static const $core.String defaultHost = '';

  /// OAuth scopes needed for the client.
  static const $core.List<$core.String> oauthScopes = [
    '',
  ];

  TelemetryCollectorClient(super.channel, {super.options, super.interceptors});

  $grpc.ResponseFuture<$0.StreamSummary> streamTelemetry(
    $async.Stream<$0.TelemetryEvent> request, {
    $grpc.CallOptions? options,
  }) {
    return $createStreamingCall(_$streamTelemetry, request, options: options)
        .single;
  }

  // method descriptors

  static final _$streamTelemetry =
      $grpc.ClientMethod<$0.TelemetryEvent, $0.StreamSummary>(
          '/traceforge.v1.TelemetryCollector/StreamTelemetry',
          ($0.TelemetryEvent value) => value.writeToBuffer(),
          $0.StreamSummary.fromBuffer);
}

@$pb.GrpcServiceName('traceforge.v1.TelemetryCollector')
abstract class TelemetryCollectorServiceBase extends $grpc.Service {
  $core.String get $name => 'traceforge.v1.TelemetryCollector';

  TelemetryCollectorServiceBase() {
    $addMethod($grpc.ServiceMethod<$0.TelemetryEvent, $0.StreamSummary>(
        'StreamTelemetry',
        streamTelemetry,
        true,
        false,
        ($core.List<$core.int> value) => $0.TelemetryEvent.fromBuffer(value),
        ($0.StreamSummary value) => value.writeToBuffer()));
  }

  $async.Future<$0.StreamSummary> streamTelemetry(
      $grpc.ServiceCall call, $async.Stream<$0.TelemetryEvent> request);
}
