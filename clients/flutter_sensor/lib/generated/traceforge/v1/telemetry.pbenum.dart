// This is a generated file - do not edit.
//
// Generated from traceforge/v1/telemetry.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:core' as $core;

import 'package:protobuf/protobuf.dart' as $pb;

/// Identifies the clock that produced source_timestamp_ns. Clock domains must
/// not be compared until an explicit offset has been estimated.
class ClockDomain extends $pb.ProtobufEnum {
  static const ClockDomain CLOCK_DOMAIN_UNSPECIFIED =
      ClockDomain._(0, _omitEnumNames ? '' : 'CLOCK_DOMAIN_UNSPECIFIED');
  static const ClockDomain CLOCK_DOMAIN_UTC =
      ClockDomain._(1, _omitEnumNames ? '' : 'CLOCK_DOMAIN_UTC');
  static const ClockDomain CLOCK_DOMAIN_DEVICE_MONOTONIC =
      ClockDomain._(2, _omitEnumNames ? '' : 'CLOCK_DOMAIN_DEVICE_MONOTONIC');

  static const $core.List<ClockDomain> values = <ClockDomain>[
    CLOCK_DOMAIN_UNSPECIFIED,
    CLOCK_DOMAIN_UTC,
    CLOCK_DOMAIN_DEVICE_MONOTONIC,
  ];

  static final $core.List<ClockDomain?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 2);
  static ClockDomain? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const ClockDomain._(super.value, super.name);
}

class ControllerStatus_State extends $pb.ProtobufEnum {
  static const ControllerStatus_State STATE_UNSPECIFIED =
      ControllerStatus_State._(0, _omitEnumNames ? '' : 'STATE_UNSPECIFIED');
  static const ControllerStatus_State STATE_INITIALIZING =
      ControllerStatus_State._(1, _omitEnumNames ? '' : 'STATE_INITIALIZING');
  static const ControllerStatus_State STATE_READY =
      ControllerStatus_State._(2, _omitEnumNames ? '' : 'STATE_READY');
  static const ControllerStatus_State STATE_DEGRADED =
      ControllerStatus_State._(3, _omitEnumNames ? '' : 'STATE_DEGRADED');
  static const ControllerStatus_State STATE_STOPPED =
      ControllerStatus_State._(4, _omitEnumNames ? '' : 'STATE_STOPPED');

  static const $core.List<ControllerStatus_State> values =
      <ControllerStatus_State>[
    STATE_UNSPECIFIED,
    STATE_INITIALIZING,
    STATE_READY,
    STATE_DEGRADED,
    STATE_STOPPED,
  ];

  static final $core.List<ControllerStatus_State?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 4);
  static ControllerStatus_State? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const ControllerStatus_State._(super.value, super.name);
}

class FaultEvent_Severity extends $pb.ProtobufEnum {
  static const FaultEvent_Severity SEVERITY_UNSPECIFIED =
      FaultEvent_Severity._(0, _omitEnumNames ? '' : 'SEVERITY_UNSPECIFIED');
  static const FaultEvent_Severity SEVERITY_WARNING =
      FaultEvent_Severity._(1, _omitEnumNames ? '' : 'SEVERITY_WARNING');
  static const FaultEvent_Severity SEVERITY_ERROR =
      FaultEvent_Severity._(2, _omitEnumNames ? '' : 'SEVERITY_ERROR');
  static const FaultEvent_Severity SEVERITY_CRITICAL =
      FaultEvent_Severity._(3, _omitEnumNames ? '' : 'SEVERITY_CRITICAL');

  static const $core.List<FaultEvent_Severity> values = <FaultEvent_Severity>[
    SEVERITY_UNSPECIFIED,
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    SEVERITY_CRITICAL,
  ];

  static final $core.List<FaultEvent_Severity?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 3);
  static FaultEvent_Severity? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const FaultEvent_Severity._(super.value, super.name);
}

const $core.bool _omitEnumNames =
    $core.bool.fromEnvironment('protobuf.omit_enum_names');
