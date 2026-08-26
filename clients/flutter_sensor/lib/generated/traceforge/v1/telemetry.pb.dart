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

import 'package:fixnum/fixnum.dart' as $fixnum;
import 'package:protobuf/protobuf.dart' as $pb;

import 'telemetry.pbenum.dart';

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

export 'telemetry.pbenum.dart';

class Vector3 extends $pb.GeneratedMessage {
  factory Vector3({
    $core.double? x,
    $core.double? y,
    $core.double? z,
  }) {
    final result = create();
    if (x != null) result.x = x;
    if (y != null) result.y = y;
    if (z != null) result.z = z;
    return result;
  }

  Vector3._();

  factory Vector3.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromBuffer(data, registry);
  factory Vector3.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'Vector3',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'traceforge.v1'),
      createEmptyInstance: create)
    ..aD(1, _omitFieldNames ? '' : 'x')
    ..aD(2, _omitFieldNames ? '' : 'y')
    ..aD(3, _omitFieldNames ? '' : 'z')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Vector3 clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Vector3 copyWith(void Function(Vector3) updates) =>
      super.copyWith((message) => updates(message as Vector3)) as Vector3;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static Vector3 create() => Vector3._();
  @$core.override
  Vector3 createEmptyInstance() => create();
  @$core.pragma('dart2js:noInline')
  static Vector3 getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Vector3>(create);
  static Vector3? _defaultInstance;

  @$pb.TagNumber(1)
  $core.double get x => $_getN(0);
  @$pb.TagNumber(1)
  set x($core.double value) => $_setDouble(0, value);
  @$pb.TagNumber(1)
  $core.bool hasX() => $_has(0);
  @$pb.TagNumber(1)
  void clearX() => $_clearField(1);

  @$pb.TagNumber(2)
  $core.double get y => $_getN(1);
  @$pb.TagNumber(2)
  set y($core.double value) => $_setDouble(1, value);
  @$pb.TagNumber(2)
  $core.bool hasY() => $_has(1);
  @$pb.TagNumber(2)
  void clearY() => $_clearField(2);

  @$pb.TagNumber(3)
  $core.double get z => $_getN(2);
  @$pb.TagNumber(3)
  set z($core.double value) => $_setDouble(2, value);
  @$pb.TagNumber(3)
  $core.bool hasZ() => $_has(2);
  @$pb.TagNumber(3)
  void clearZ() => $_clearField(3);
}

class GpsFix extends $pb.GeneratedMessage {
  factory GpsFix({
    $core.double? latitudeDegrees,
    $core.double? longitudeDegrees,
    $core.double? horizontalAccuracyMeters,
    $core.bool? isMocked,
  }) {
    final result = create();
    if (latitudeDegrees != null) result.latitudeDegrees = latitudeDegrees;
    if (longitudeDegrees != null) result.longitudeDegrees = longitudeDegrees;
    if (horizontalAccuracyMeters != null)
      result.horizontalAccuracyMeters = horizontalAccuracyMeters;
    if (isMocked != null) result.isMocked = isMocked;
    return result;
  }

  GpsFix._();

  factory GpsFix.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromBuffer(data, registry);
  factory GpsFix.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GpsFix',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'traceforge.v1'),
      createEmptyInstance: create)
    ..aD(1, _omitFieldNames ? '' : 'latitudeDegrees')
    ..aD(2, _omitFieldNames ? '' : 'longitudeDegrees')
    ..aD(3, _omitFieldNames ? '' : 'horizontalAccuracyMeters')
    ..aOB(4, _omitFieldNames ? '' : 'isMocked')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GpsFix clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GpsFix copyWith(void Function(GpsFix) updates) =>
      super.copyWith((message) => updates(message as GpsFix)) as GpsFix;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static GpsFix create() => GpsFix._();
  @$core.override
  GpsFix createEmptyInstance() => create();
  @$core.pragma('dart2js:noInline')
  static GpsFix getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<GpsFix>(create);
  static GpsFix? _defaultInstance;

  @$pb.TagNumber(1)
  $core.double get latitudeDegrees => $_getN(0);
  @$pb.TagNumber(1)
  set latitudeDegrees($core.double value) => $_setDouble(0, value);
  @$pb.TagNumber(1)
  $core.bool hasLatitudeDegrees() => $_has(0);
  @$pb.TagNumber(1)
  void clearLatitudeDegrees() => $_clearField(1);

  @$pb.TagNumber(2)
  $core.double get longitudeDegrees => $_getN(1);
  @$pb.TagNumber(2)
  set longitudeDegrees($core.double value) => $_setDouble(1, value);
  @$pb.TagNumber(2)
  $core.bool hasLongitudeDegrees() => $_has(1);
  @$pb.TagNumber(2)
  void clearLongitudeDegrees() => $_clearField(2);

  @$pb.TagNumber(3)
  $core.double get horizontalAccuracyMeters => $_getN(2);
  @$pb.TagNumber(3)
  set horizontalAccuracyMeters($core.double value) => $_setDouble(2, value);
  @$pb.TagNumber(3)
  $core.bool hasHorizontalAccuracyMeters() => $_has(2);
  @$pb.TagNumber(3)
  void clearHorizontalAccuracyMeters() => $_clearField(3);

  @$pb.TagNumber(4)
  $core.bool get isMocked => $_getBF(3);
  @$pb.TagNumber(4)
  set isMocked($core.bool value) => $_setBool(3, value);
  @$pb.TagNumber(4)
  $core.bool hasIsMocked() => $_has(3);
  @$pb.TagNumber(4)
  void clearIsMocked() => $_clearField(4);
}

class Temperature extends $pb.GeneratedMessage {
  factory Temperature({
    $core.double? degreesCelsius,
  }) {
    final result = create();
    if (degreesCelsius != null) result.degreesCelsius = degreesCelsius;
    return result;
  }

  Temperature._();

  factory Temperature.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromBuffer(data, registry);
  factory Temperature.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'Temperature',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'traceforge.v1'),
      createEmptyInstance: create)
    ..aD(1, _omitFieldNames ? '' : 'degreesCelsius')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Temperature clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Temperature copyWith(void Function(Temperature) updates) =>
      super.copyWith((message) => updates(message as Temperature))
          as Temperature;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static Temperature create() => Temperature._();
  @$core.override
  Temperature createEmptyInstance() => create();
  @$core.pragma('dart2js:noInline')
  static Temperature getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<Temperature>(create);
  static Temperature? _defaultInstance;

  @$pb.TagNumber(1)
  $core.double get degreesCelsius => $_getN(0);
  @$pb.TagNumber(1)
  set degreesCelsius($core.double value) => $_setDouble(0, value);
  @$pb.TagNumber(1)
  $core.bool hasDegreesCelsius() => $_has(0);
  @$pb.TagNumber(1)
  void clearDegreesCelsius() => $_clearField(1);
}

class CameraMetadata extends $pb.GeneratedMessage {
  factory CameraMetadata({
    $fixnum.Int64? frameNumber,
    $core.int? widthPixels,
    $core.int? heightPixels,
    $fixnum.Int64? exposureTimeNs,
  }) {
    final result = create();
    if (frameNumber != null) result.frameNumber = frameNumber;
    if (widthPixels != null) result.widthPixels = widthPixels;
    if (heightPixels != null) result.heightPixels = heightPixels;
    if (exposureTimeNs != null) result.exposureTimeNs = exposureTimeNs;
    return result;
  }

  CameraMetadata._();

  factory CameraMetadata.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromBuffer(data, registry);
  factory CameraMetadata.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'CameraMetadata',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'traceforge.v1'),
      createEmptyInstance: create)
    ..a<$fixnum.Int64>(
        1, _omitFieldNames ? '' : 'frameNumber', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..aI(2, _omitFieldNames ? '' : 'widthPixels',
        fieldType: $pb.PbFieldType.OU3)
    ..aI(3, _omitFieldNames ? '' : 'heightPixels',
        fieldType: $pb.PbFieldType.OU3)
    ..a<$fixnum.Int64>(
        4, _omitFieldNames ? '' : 'exposureTimeNs', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  CameraMetadata clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  CameraMetadata copyWith(void Function(CameraMetadata) updates) =>
      super.copyWith((message) => updates(message as CameraMetadata))
          as CameraMetadata;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static CameraMetadata create() => CameraMetadata._();
  @$core.override
  CameraMetadata createEmptyInstance() => create();
  @$core.pragma('dart2js:noInline')
  static CameraMetadata getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<CameraMetadata>(create);
  static CameraMetadata? _defaultInstance;

  @$pb.TagNumber(1)
  $fixnum.Int64 get frameNumber => $_getI64(0);
  @$pb.TagNumber(1)
  set frameNumber($fixnum.Int64 value) => $_setInt64(0, value);
  @$pb.TagNumber(1)
  $core.bool hasFrameNumber() => $_has(0);
  @$pb.TagNumber(1)
  void clearFrameNumber() => $_clearField(1);

  @$pb.TagNumber(2)
  $core.int get widthPixels => $_getIZ(1);
  @$pb.TagNumber(2)
  set widthPixels($core.int value) => $_setUnsignedInt32(1, value);
  @$pb.TagNumber(2)
  $core.bool hasWidthPixels() => $_has(1);
  @$pb.TagNumber(2)
  void clearWidthPixels() => $_clearField(2);

  @$pb.TagNumber(3)
  $core.int get heightPixels => $_getIZ(2);
  @$pb.TagNumber(3)
  set heightPixels($core.int value) => $_setUnsignedInt32(2, value);
  @$pb.TagNumber(3)
  $core.bool hasHeightPixels() => $_has(2);
  @$pb.TagNumber(3)
  void clearHeightPixels() => $_clearField(3);

  @$pb.TagNumber(4)
  $fixnum.Int64 get exposureTimeNs => $_getI64(3);
  @$pb.TagNumber(4)
  set exposureTimeNs($fixnum.Int64 value) => $_setInt64(3, value);
  @$pb.TagNumber(4)
  $core.bool hasExposureTimeNs() => $_has(3);
  @$pb.TagNumber(4)
  void clearExposureTimeNs() => $_clearField(4);
}

class ControllerStatus extends $pb.GeneratedMessage {
  factory ControllerStatus({
    ControllerStatus_State? state,
    $core.String? detail,
  }) {
    final result = create();
    if (state != null) result.state = state;
    if (detail != null) result.detail = detail;
    return result;
  }

  ControllerStatus._();

  factory ControllerStatus.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromBuffer(data, registry);
  factory ControllerStatus.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'ControllerStatus',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'traceforge.v1'),
      createEmptyInstance: create)
    ..aE<ControllerStatus_State>(1, _omitFieldNames ? '' : 'state',
        enumValues: ControllerStatus_State.values)
    ..aOS(2, _omitFieldNames ? '' : 'detail')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ControllerStatus clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ControllerStatus copyWith(void Function(ControllerStatus) updates) =>
      super.copyWith((message) => updates(message as ControllerStatus))
          as ControllerStatus;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static ControllerStatus create() => ControllerStatus._();
  @$core.override
  ControllerStatus createEmptyInstance() => create();
  @$core.pragma('dart2js:noInline')
  static ControllerStatus getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<ControllerStatus>(create);
  static ControllerStatus? _defaultInstance;

  @$pb.TagNumber(1)
  ControllerStatus_State get state => $_getN(0);
  @$pb.TagNumber(1)
  set state(ControllerStatus_State value) => $_setField(1, value);
  @$pb.TagNumber(1)
  $core.bool hasState() => $_has(0);
  @$pb.TagNumber(1)
  void clearState() => $_clearField(1);

  @$pb.TagNumber(2)
  $core.String get detail => $_getSZ(1);
  @$pb.TagNumber(2)
  set detail($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasDetail() => $_has(1);
  @$pb.TagNumber(2)
  void clearDetail() => $_clearField(2);
}

class FaultEvent extends $pb.GeneratedMessage {
  factory FaultEvent({
    FaultEvent_Severity? severity,
    $core.int? code,
    $core.String? message,
  }) {
    final result = create();
    if (severity != null) result.severity = severity;
    if (code != null) result.code = code;
    if (message != null) result.message = message;
    return result;
  }

  FaultEvent._();

  factory FaultEvent.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromBuffer(data, registry);
  factory FaultEvent.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'FaultEvent',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'traceforge.v1'),
      createEmptyInstance: create)
    ..aE<FaultEvent_Severity>(1, _omitFieldNames ? '' : 'severity',
        enumValues: FaultEvent_Severity.values)
    ..aI(2, _omitFieldNames ? '' : 'code', fieldType: $pb.PbFieldType.OU3)
    ..aOS(3, _omitFieldNames ? '' : 'message')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  FaultEvent clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  FaultEvent copyWith(void Function(FaultEvent) updates) =>
      super.copyWith((message) => updates(message as FaultEvent)) as FaultEvent;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static FaultEvent create() => FaultEvent._();
  @$core.override
  FaultEvent createEmptyInstance() => create();
  @$core.pragma('dart2js:noInline')
  static FaultEvent getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<FaultEvent>(create);
  static FaultEvent? _defaultInstance;

  @$pb.TagNumber(1)
  FaultEvent_Severity get severity => $_getN(0);
  @$pb.TagNumber(1)
  set severity(FaultEvent_Severity value) => $_setField(1, value);
  @$pb.TagNumber(1)
  $core.bool hasSeverity() => $_has(0);
  @$pb.TagNumber(1)
  void clearSeverity() => $_clearField(1);

  @$pb.TagNumber(2)
  $core.int get code => $_getIZ(1);
  @$pb.TagNumber(2)
  set code($core.int value) => $_setUnsignedInt32(1, value);
  @$pb.TagNumber(2)
  $core.bool hasCode() => $_has(1);
  @$pb.TagNumber(2)
  void clearCode() => $_clearField(2);

  @$pb.TagNumber(3)
  $core.String get message => $_getSZ(2);
  @$pb.TagNumber(3)
  set message($core.String value) => $_setString(2, value);
  @$pb.TagNumber(3)
  $core.bool hasMessage() => $_has(2);
  @$pb.TagNumber(3)
  void clearMessage() => $_clearField(3);
}

enum TelemetryEvent_Payload {
  accelerometer,
  gyroscope,
  gps,
  temperature,
  camera,
  controllerStatus,
  fault,
  notSet
}

class TelemetryEvent extends $pb.GeneratedMessage {
  factory TelemetryEvent({
    $core.int? schemaVersion,
    $core.String? producerId,
    $fixnum.Int64? sequenceNumber,
    $fixnum.Int64? sourceTimestampNs,
    ClockDomain? sourceClock,
    Vector3? accelerometer,
    Vector3? gyroscope,
    GpsFix? gps,
    Temperature? temperature,
    CameraMetadata? camera,
    ControllerStatus? controllerStatus,
    FaultEvent? fault,
  }) {
    final result = create();
    if (schemaVersion != null) result.schemaVersion = schemaVersion;
    if (producerId != null) result.producerId = producerId;
    if (sequenceNumber != null) result.sequenceNumber = sequenceNumber;
    if (sourceTimestampNs != null) result.sourceTimestampNs = sourceTimestampNs;
    if (sourceClock != null) result.sourceClock = sourceClock;
    if (accelerometer != null) result.accelerometer = accelerometer;
    if (gyroscope != null) result.gyroscope = gyroscope;
    if (gps != null) result.gps = gps;
    if (temperature != null) result.temperature = temperature;
    if (camera != null) result.camera = camera;
    if (controllerStatus != null) result.controllerStatus = controllerStatus;
    if (fault != null) result.fault = fault;
    return result;
  }

  TelemetryEvent._();

  factory TelemetryEvent.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromBuffer(data, registry);
  factory TelemetryEvent.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromJson(json, registry);

  static const $core.Map<$core.int, TelemetryEvent_Payload>
      _TelemetryEvent_PayloadByTag = {
    10: TelemetryEvent_Payload.accelerometer,
    11: TelemetryEvent_Payload.gyroscope,
    12: TelemetryEvent_Payload.gps,
    13: TelemetryEvent_Payload.temperature,
    14: TelemetryEvent_Payload.camera,
    15: TelemetryEvent_Payload.controllerStatus,
    16: TelemetryEvent_Payload.fault,
    0: TelemetryEvent_Payload.notSet
  };
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'TelemetryEvent',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'traceforge.v1'),
      createEmptyInstance: create)
    ..oo(0, [10, 11, 12, 13, 14, 15, 16])
    ..aI(1, _omitFieldNames ? '' : 'schemaVersion',
        fieldType: $pb.PbFieldType.OU3)
    ..aOS(2, _omitFieldNames ? '' : 'producerId')
    ..a<$fixnum.Int64>(
        3, _omitFieldNames ? '' : 'sequenceNumber', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..aInt64(4, _omitFieldNames ? '' : 'sourceTimestampNs')
    ..aE<ClockDomain>(5, _omitFieldNames ? '' : 'sourceClock',
        enumValues: ClockDomain.values)
    ..aOM<Vector3>(10, _omitFieldNames ? '' : 'accelerometer',
        subBuilder: Vector3.create)
    ..aOM<Vector3>(11, _omitFieldNames ? '' : 'gyroscope',
        subBuilder: Vector3.create)
    ..aOM<GpsFix>(12, _omitFieldNames ? '' : 'gps', subBuilder: GpsFix.create)
    ..aOM<Temperature>(13, _omitFieldNames ? '' : 'temperature',
        subBuilder: Temperature.create)
    ..aOM<CameraMetadata>(14, _omitFieldNames ? '' : 'camera',
        subBuilder: CameraMetadata.create)
    ..aOM<ControllerStatus>(15, _omitFieldNames ? '' : 'controllerStatus',
        subBuilder: ControllerStatus.create)
    ..aOM<FaultEvent>(16, _omitFieldNames ? '' : 'fault',
        subBuilder: FaultEvent.create)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  TelemetryEvent clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  TelemetryEvent copyWith(void Function(TelemetryEvent) updates) =>
      super.copyWith((message) => updates(message as TelemetryEvent))
          as TelemetryEvent;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static TelemetryEvent create() => TelemetryEvent._();
  @$core.override
  TelemetryEvent createEmptyInstance() => create();
  @$core.pragma('dart2js:noInline')
  static TelemetryEvent getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<TelemetryEvent>(create);
  static TelemetryEvent? _defaultInstance;

  @$pb.TagNumber(10)
  @$pb.TagNumber(11)
  @$pb.TagNumber(12)
  @$pb.TagNumber(13)
  @$pb.TagNumber(14)
  @$pb.TagNumber(15)
  @$pb.TagNumber(16)
  TelemetryEvent_Payload whichPayload() =>
      _TelemetryEvent_PayloadByTag[$_whichOneof(0)]!;
  @$pb.TagNumber(10)
  @$pb.TagNumber(11)
  @$pb.TagNumber(12)
  @$pb.TagNumber(13)
  @$pb.TagNumber(14)
  @$pb.TagNumber(15)
  @$pb.TagNumber(16)
  void clearPayload() => $_clearField($_whichOneof(0));

  @$pb.TagNumber(1)
  $core.int get schemaVersion => $_getIZ(0);
  @$pb.TagNumber(1)
  set schemaVersion($core.int value) => $_setUnsignedInt32(0, value);
  @$pb.TagNumber(1)
  $core.bool hasSchemaVersion() => $_has(0);
  @$pb.TagNumber(1)
  void clearSchemaVersion() => $_clearField(1);

  @$pb.TagNumber(2)
  $core.String get producerId => $_getSZ(1);
  @$pb.TagNumber(2)
  set producerId($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasProducerId() => $_has(1);
  @$pb.TagNumber(2)
  void clearProducerId() => $_clearField(2);

  @$pb.TagNumber(3)
  $fixnum.Int64 get sequenceNumber => $_getI64(2);
  @$pb.TagNumber(3)
  set sequenceNumber($fixnum.Int64 value) => $_setInt64(2, value);
  @$pb.TagNumber(3)
  $core.bool hasSequenceNumber() => $_has(2);
  @$pb.TagNumber(3)
  void clearSequenceNumber() => $_clearField(3);

  @$pb.TagNumber(4)
  $fixnum.Int64 get sourceTimestampNs => $_getI64(3);
  @$pb.TagNumber(4)
  set sourceTimestampNs($fixnum.Int64 value) => $_setInt64(3, value);
  @$pb.TagNumber(4)
  $core.bool hasSourceTimestampNs() => $_has(3);
  @$pb.TagNumber(4)
  void clearSourceTimestampNs() => $_clearField(4);

  @$pb.TagNumber(5)
  ClockDomain get sourceClock => $_getN(4);
  @$pb.TagNumber(5)
  set sourceClock(ClockDomain value) => $_setField(5, value);
  @$pb.TagNumber(5)
  $core.bool hasSourceClock() => $_has(4);
  @$pb.TagNumber(5)
  void clearSourceClock() => $_clearField(5);

  @$pb.TagNumber(10)
  Vector3 get accelerometer => $_getN(5);
  @$pb.TagNumber(10)
  set accelerometer(Vector3 value) => $_setField(10, value);
  @$pb.TagNumber(10)
  $core.bool hasAccelerometer() => $_has(5);
  @$pb.TagNumber(10)
  void clearAccelerometer() => $_clearField(10);
  @$pb.TagNumber(10)
  Vector3 ensureAccelerometer() => $_ensure(5);

  @$pb.TagNumber(11)
  Vector3 get gyroscope => $_getN(6);
  @$pb.TagNumber(11)
  set gyroscope(Vector3 value) => $_setField(11, value);
  @$pb.TagNumber(11)
  $core.bool hasGyroscope() => $_has(6);
  @$pb.TagNumber(11)
  void clearGyroscope() => $_clearField(11);
  @$pb.TagNumber(11)
  Vector3 ensureGyroscope() => $_ensure(6);

  @$pb.TagNumber(12)
  GpsFix get gps => $_getN(7);
  @$pb.TagNumber(12)
  set gps(GpsFix value) => $_setField(12, value);
  @$pb.TagNumber(12)
  $core.bool hasGps() => $_has(7);
  @$pb.TagNumber(12)
  void clearGps() => $_clearField(12);
  @$pb.TagNumber(12)
  GpsFix ensureGps() => $_ensure(7);

  @$pb.TagNumber(13)
  Temperature get temperature => $_getN(8);
  @$pb.TagNumber(13)
  set temperature(Temperature value) => $_setField(13, value);
  @$pb.TagNumber(13)
  $core.bool hasTemperature() => $_has(8);
  @$pb.TagNumber(13)
  void clearTemperature() => $_clearField(13);
  @$pb.TagNumber(13)
  Temperature ensureTemperature() => $_ensure(8);

  @$pb.TagNumber(14)
  CameraMetadata get camera => $_getN(9);
  @$pb.TagNumber(14)
  set camera(CameraMetadata value) => $_setField(14, value);
  @$pb.TagNumber(14)
  $core.bool hasCamera() => $_has(9);
  @$pb.TagNumber(14)
  void clearCamera() => $_clearField(14);
  @$pb.TagNumber(14)
  CameraMetadata ensureCamera() => $_ensure(9);

  @$pb.TagNumber(15)
  ControllerStatus get controllerStatus => $_getN(10);
  @$pb.TagNumber(15)
  set controllerStatus(ControllerStatus value) => $_setField(15, value);
  @$pb.TagNumber(15)
  $core.bool hasControllerStatus() => $_has(10);
  @$pb.TagNumber(15)
  void clearControllerStatus() => $_clearField(15);
  @$pb.TagNumber(15)
  ControllerStatus ensureControllerStatus() => $_ensure(10);

  @$pb.TagNumber(16)
  FaultEvent get fault => $_getN(11);
  @$pb.TagNumber(16)
  set fault(FaultEvent value) => $_setField(16, value);
  @$pb.TagNumber(16)
  $core.bool hasFault() => $_has(11);
  @$pb.TagNumber(16)
  void clearFault() => $_clearField(16);
  @$pb.TagNumber(16)
  FaultEvent ensureFault() => $_ensure(11);
}

class StreamSummary extends $pb.GeneratedMessage {
  factory StreamSummary({
    $core.String? producerId,
    $fixnum.Int64? acceptedEvents,
    $fixnum.Int64? firstSequenceNumber,
    $fixnum.Int64? lastSequenceNumber,
    $fixnum.Int64? sequenceGaps,
  }) {
    final result = create();
    if (producerId != null) result.producerId = producerId;
    if (acceptedEvents != null) result.acceptedEvents = acceptedEvents;
    if (firstSequenceNumber != null)
      result.firstSequenceNumber = firstSequenceNumber;
    if (lastSequenceNumber != null)
      result.lastSequenceNumber = lastSequenceNumber;
    if (sequenceGaps != null) result.sequenceGaps = sequenceGaps;
    return result;
  }

  StreamSummary._();

  factory StreamSummary.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromBuffer(data, registry);
  factory StreamSummary.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      create()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'StreamSummary',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'traceforge.v1'),
      createEmptyInstance: create)
    ..aOS(1, _omitFieldNames ? '' : 'producerId')
    ..a<$fixnum.Int64>(
        2, _omitFieldNames ? '' : 'acceptedEvents', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..a<$fixnum.Int64>(
        3, _omitFieldNames ? '' : 'firstSequenceNumber', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..a<$fixnum.Int64>(
        4, _omitFieldNames ? '' : 'lastSequenceNumber', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..a<$fixnum.Int64>(
        5, _omitFieldNames ? '' : 'sequenceGaps', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  StreamSummary clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  StreamSummary copyWith(void Function(StreamSummary) updates) =>
      super.copyWith((message) => updates(message as StreamSummary))
          as StreamSummary;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static StreamSummary create() => StreamSummary._();
  @$core.override
  StreamSummary createEmptyInstance() => create();
  @$core.pragma('dart2js:noInline')
  static StreamSummary getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<StreamSummary>(create);
  static StreamSummary? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get producerId => $_getSZ(0);
  @$pb.TagNumber(1)
  set producerId($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasProducerId() => $_has(0);
  @$pb.TagNumber(1)
  void clearProducerId() => $_clearField(1);

  @$pb.TagNumber(2)
  $fixnum.Int64 get acceptedEvents => $_getI64(1);
  @$pb.TagNumber(2)
  set acceptedEvents($fixnum.Int64 value) => $_setInt64(1, value);
  @$pb.TagNumber(2)
  $core.bool hasAcceptedEvents() => $_has(1);
  @$pb.TagNumber(2)
  void clearAcceptedEvents() => $_clearField(2);

  @$pb.TagNumber(3)
  $fixnum.Int64 get firstSequenceNumber => $_getI64(2);
  @$pb.TagNumber(3)
  set firstSequenceNumber($fixnum.Int64 value) => $_setInt64(2, value);
  @$pb.TagNumber(3)
  $core.bool hasFirstSequenceNumber() => $_has(2);
  @$pb.TagNumber(3)
  void clearFirstSequenceNumber() => $_clearField(3);

  @$pb.TagNumber(4)
  $fixnum.Int64 get lastSequenceNumber => $_getI64(3);
  @$pb.TagNumber(4)
  set lastSequenceNumber($fixnum.Int64 value) => $_setInt64(3, value);
  @$pb.TagNumber(4)
  $core.bool hasLastSequenceNumber() => $_has(3);
  @$pb.TagNumber(4)
  void clearLastSequenceNumber() => $_clearField(4);

  @$pb.TagNumber(5)
  $fixnum.Int64 get sequenceGaps => $_getI64(4);
  @$pb.TagNumber(5)
  set sequenceGaps($fixnum.Int64 value) => $_setInt64(4, value);
  @$pb.TagNumber(5)
  $core.bool hasSequenceGaps() => $_has(4);
  @$pb.TagNumber(5)
  void clearSequenceGaps() => $_clearField(5);
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
