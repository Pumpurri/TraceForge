// This is a generated file - do not edit.
//
// Generated from traceforge/v1/telemetry.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports
// ignore_for_file: unused_import

import 'dart:convert' as $convert;
import 'dart:core' as $core;
import 'dart:typed_data' as $typed_data;

@$core.Deprecated('Use clockDomainDescriptor instead')
const ClockDomain$json = {
  '1': 'ClockDomain',
  '2': [
    {'1': 'CLOCK_DOMAIN_UNSPECIFIED', '2': 0},
    {'1': 'CLOCK_DOMAIN_UTC', '2': 1},
    {'1': 'CLOCK_DOMAIN_DEVICE_MONOTONIC', '2': 2},
  ],
};

/// Descriptor for `ClockDomain`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List clockDomainDescriptor = $convert.base64Decode(
    'CgtDbG9ja0RvbWFpbhIcChhDTE9DS19ET01BSU5fVU5TUEVDSUZJRUQQABIUChBDTE9DS19ET0'
    '1BSU5fVVRDEAESIQodQ0xPQ0tfRE9NQUlOX0RFVklDRV9NT05PVE9OSUMQAg==');

@$core.Deprecated('Use vector3Descriptor instead')
const Vector3$json = {
  '1': 'Vector3',
  '2': [
    {'1': 'x', '3': 1, '4': 1, '5': 1, '10': 'x'},
    {'1': 'y', '3': 2, '4': 1, '5': 1, '10': 'y'},
    {'1': 'z', '3': 3, '4': 1, '5': 1, '10': 'z'},
  ],
};

/// Descriptor for `Vector3`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List vector3Descriptor = $convert.base64Decode(
    'CgdWZWN0b3IzEgwKAXgYASABKAFSAXgSDAoBeRgCIAEoAVIBeRIMCgF6GAMgASgBUgF6');

@$core.Deprecated('Use gpsFixDescriptor instead')
const GpsFix$json = {
  '1': 'GpsFix',
  '2': [
    {'1': 'latitude_degrees', '3': 1, '4': 1, '5': 1, '10': 'latitudeDegrees'},
    {
      '1': 'longitude_degrees',
      '3': 2,
      '4': 1,
      '5': 1,
      '10': 'longitudeDegrees'
    },
    {
      '1': 'horizontal_accuracy_meters',
      '3': 3,
      '4': 1,
      '5': 1,
      '10': 'horizontalAccuracyMeters'
    },
    {'1': 'is_mocked', '3': 4, '4': 1, '5': 8, '10': 'isMocked'},
  ],
};

/// Descriptor for `GpsFix`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List gpsFixDescriptor = $convert.base64Decode(
    'CgZHcHNGaXgSKQoQbGF0aXR1ZGVfZGVncmVlcxgBIAEoAVIPbGF0aXR1ZGVEZWdyZWVzEisKEW'
    'xvbmdpdHVkZV9kZWdyZWVzGAIgASgBUhBsb25naXR1ZGVEZWdyZWVzEjwKGmhvcml6b250YWxf'
    'YWNjdXJhY3lfbWV0ZXJzGAMgASgBUhhob3Jpem9udGFsQWNjdXJhY3lNZXRlcnMSGwoJaXNfbW'
    '9ja2VkGAQgASgIUghpc01vY2tlZA==');

@$core.Deprecated('Use temperatureDescriptor instead')
const Temperature$json = {
  '1': 'Temperature',
  '2': [
    {'1': 'degrees_celsius', '3': 1, '4': 1, '5': 1, '10': 'degreesCelsius'},
  ],
};

/// Descriptor for `Temperature`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List temperatureDescriptor = $convert.base64Decode(
    'CgtUZW1wZXJhdHVyZRInCg9kZWdyZWVzX2NlbHNpdXMYASABKAFSDmRlZ3JlZXNDZWxzaXVz');

@$core.Deprecated('Use cameraMetadataDescriptor instead')
const CameraMetadata$json = {
  '1': 'CameraMetadata',
  '2': [
    {'1': 'frame_number', '3': 1, '4': 1, '5': 4, '10': 'frameNumber'},
    {'1': 'width_pixels', '3': 2, '4': 1, '5': 13, '10': 'widthPixels'},
    {'1': 'height_pixels', '3': 3, '4': 1, '5': 13, '10': 'heightPixels'},
    {'1': 'exposure_time_ns', '3': 4, '4': 1, '5': 4, '10': 'exposureTimeNs'},
  ],
};

/// Descriptor for `CameraMetadata`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List cameraMetadataDescriptor = $convert.base64Decode(
    'Cg5DYW1lcmFNZXRhZGF0YRIhCgxmcmFtZV9udW1iZXIYASABKARSC2ZyYW1lTnVtYmVyEiEKDH'
    'dpZHRoX3BpeGVscxgCIAEoDVILd2lkdGhQaXhlbHMSIwoNaGVpZ2h0X3BpeGVscxgDIAEoDVIM'
    'aGVpZ2h0UGl4ZWxzEigKEGV4cG9zdXJlX3RpbWVfbnMYBCABKARSDmV4cG9zdXJlVGltZU5z');

@$core.Deprecated('Use controllerStatusDescriptor instead')
const ControllerStatus$json = {
  '1': 'ControllerStatus',
  '2': [
    {
      '1': 'state',
      '3': 1,
      '4': 1,
      '5': 14,
      '6': '.traceforge.v1.ControllerStatus.State',
      '10': 'state'
    },
    {'1': 'detail', '3': 2, '4': 1, '5': 9, '10': 'detail'},
  ],
  '4': [ControllerStatus_State$json],
};

@$core.Deprecated('Use controllerStatusDescriptor instead')
const ControllerStatus_State$json = {
  '1': 'State',
  '2': [
    {'1': 'STATE_UNSPECIFIED', '2': 0},
    {'1': 'STATE_INITIALIZING', '2': 1},
    {'1': 'STATE_READY', '2': 2},
    {'1': 'STATE_DEGRADED', '2': 3},
    {'1': 'STATE_STOPPED', '2': 4},
  ],
};

/// Descriptor for `ControllerStatus`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List controllerStatusDescriptor = $convert.base64Decode(
    'ChBDb250cm9sbGVyU3RhdHVzEjsKBXN0YXRlGAEgASgOMiUudHJhY2Vmb3JnZS52MS5Db250cm'
    '9sbGVyU3RhdHVzLlN0YXRlUgVzdGF0ZRIWCgZkZXRhaWwYAiABKAlSBmRldGFpbCJuCgVTdGF0'
    'ZRIVChFTVEFURV9VTlNQRUNJRklFRBAAEhYKElNUQVRFX0lOSVRJQUxJWklORxABEg8KC1NUQV'
    'RFX1JFQURZEAISEgoOU1RBVEVfREVHUkFERUQQAxIRCg1TVEFURV9TVE9QUEVEEAQ=');

@$core.Deprecated('Use faultEventDescriptor instead')
const FaultEvent$json = {
  '1': 'FaultEvent',
  '2': [
    {
      '1': 'severity',
      '3': 1,
      '4': 1,
      '5': 14,
      '6': '.traceforge.v1.FaultEvent.Severity',
      '10': 'severity'
    },
    {'1': 'code', '3': 2, '4': 1, '5': 13, '10': 'code'},
    {'1': 'message', '3': 3, '4': 1, '5': 9, '10': 'message'},
  ],
  '4': [FaultEvent_Severity$json],
};

@$core.Deprecated('Use faultEventDescriptor instead')
const FaultEvent_Severity$json = {
  '1': 'Severity',
  '2': [
    {'1': 'SEVERITY_UNSPECIFIED', '2': 0},
    {'1': 'SEVERITY_WARNING', '2': 1},
    {'1': 'SEVERITY_ERROR', '2': 2},
    {'1': 'SEVERITY_CRITICAL', '2': 3},
  ],
};

/// Descriptor for `FaultEvent`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List faultEventDescriptor = $convert.base64Decode(
    'CgpGYXVsdEV2ZW50Ej4KCHNldmVyaXR5GAEgASgOMiIudHJhY2Vmb3JnZS52MS5GYXVsdEV2ZW'
    '50LlNldmVyaXR5UghzZXZlcml0eRISCgRjb2RlGAIgASgNUgRjb2RlEhgKB21lc3NhZ2UYAyAB'
    'KAlSB21lc3NhZ2UiZQoIU2V2ZXJpdHkSGAoUU0VWRVJJVFlfVU5TUEVDSUZJRUQQABIUChBTRV'
    'ZFUklUWV9XQVJOSU5HEAESEgoOU0VWRVJJVFlfRVJST1IQAhIVChFTRVZFUklUWV9DUklUSUNB'
    'TBAD');

@$core.Deprecated('Use telemetryEventDescriptor instead')
const TelemetryEvent$json = {
  '1': 'TelemetryEvent',
  '2': [
    {'1': 'schema_version', '3': 1, '4': 1, '5': 13, '10': 'schemaVersion'},
    {'1': 'producer_id', '3': 2, '4': 1, '5': 9, '10': 'producerId'},
    {'1': 'sequence_number', '3': 3, '4': 1, '5': 4, '10': 'sequenceNumber'},
    {
      '1': 'source_timestamp_ns',
      '3': 4,
      '4': 1,
      '5': 3,
      '10': 'sourceTimestampNs'
    },
    {
      '1': 'source_clock',
      '3': 5,
      '4': 1,
      '5': 14,
      '6': '.traceforge.v1.ClockDomain',
      '10': 'sourceClock'
    },
    {
      '1': 'accelerometer',
      '3': 10,
      '4': 1,
      '5': 11,
      '6': '.traceforge.v1.Vector3',
      '9': 0,
      '10': 'accelerometer'
    },
    {
      '1': 'gyroscope',
      '3': 11,
      '4': 1,
      '5': 11,
      '6': '.traceforge.v1.Vector3',
      '9': 0,
      '10': 'gyroscope'
    },
    {
      '1': 'gps',
      '3': 12,
      '4': 1,
      '5': 11,
      '6': '.traceforge.v1.GpsFix',
      '9': 0,
      '10': 'gps'
    },
    {
      '1': 'temperature',
      '3': 13,
      '4': 1,
      '5': 11,
      '6': '.traceforge.v1.Temperature',
      '9': 0,
      '10': 'temperature'
    },
    {
      '1': 'camera',
      '3': 14,
      '4': 1,
      '5': 11,
      '6': '.traceforge.v1.CameraMetadata',
      '9': 0,
      '10': 'camera'
    },
    {
      '1': 'controller_status',
      '3': 15,
      '4': 1,
      '5': 11,
      '6': '.traceforge.v1.ControllerStatus',
      '9': 0,
      '10': 'controllerStatus'
    },
    {
      '1': 'fault',
      '3': 16,
      '4': 1,
      '5': 11,
      '6': '.traceforge.v1.FaultEvent',
      '9': 0,
      '10': 'fault'
    },
  ],
  '8': [
    {'1': 'payload'},
  ],
};

/// Descriptor for `TelemetryEvent`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List telemetryEventDescriptor = $convert.base64Decode(
    'Cg5UZWxlbWV0cnlFdmVudBIlCg5zY2hlbWFfdmVyc2lvbhgBIAEoDVINc2NoZW1hVmVyc2lvbh'
    'IfCgtwcm9kdWNlcl9pZBgCIAEoCVIKcHJvZHVjZXJJZBInCg9zZXF1ZW5jZV9udW1iZXIYAyAB'
    'KARSDnNlcXVlbmNlTnVtYmVyEi4KE3NvdXJjZV90aW1lc3RhbXBfbnMYBCABKANSEXNvdXJjZV'
    'RpbWVzdGFtcE5zEj0KDHNvdXJjZV9jbG9jaxgFIAEoDjIaLnRyYWNlZm9yZ2UudjEuQ2xvY2tE'
    'b21haW5SC3NvdXJjZUNsb2NrEj4KDWFjY2VsZXJvbWV0ZXIYCiABKAsyFi50cmFjZWZvcmdlLn'
    'YxLlZlY3RvcjNIAFINYWNjZWxlcm9tZXRlchI2CglneXJvc2NvcGUYCyABKAsyFi50cmFjZWZv'
    'cmdlLnYxLlZlY3RvcjNIAFIJZ3lyb3Njb3BlEikKA2dwcxgMIAEoCzIVLnRyYWNlZm9yZ2Uudj'
    'EuR3BzRml4SABSA2dwcxI+Cgt0ZW1wZXJhdHVyZRgNIAEoCzIaLnRyYWNlZm9yZ2UudjEuVGVt'
    'cGVyYXR1cmVIAFILdGVtcGVyYXR1cmUSNwoGY2FtZXJhGA4gASgLMh0udHJhY2Vmb3JnZS52MS'
    '5DYW1lcmFNZXRhZGF0YUgAUgZjYW1lcmESTgoRY29udHJvbGxlcl9zdGF0dXMYDyABKAsyHy50'
    'cmFjZWZvcmdlLnYxLkNvbnRyb2xsZXJTdGF0dXNIAFIQY29udHJvbGxlclN0YXR1cxIxCgVmYX'
    'VsdBgQIAEoCzIZLnRyYWNlZm9yZ2UudjEuRmF1bHRFdmVudEgAUgVmYXVsdEIJCgdwYXlsb2Fk');

@$core.Deprecated('Use streamSummaryDescriptor instead')
const StreamSummary$json = {
  '1': 'StreamSummary',
  '2': [
    {'1': 'producer_id', '3': 1, '4': 1, '5': 9, '10': 'producerId'},
    {'1': 'accepted_events', '3': 2, '4': 1, '5': 4, '10': 'acceptedEvents'},
    {
      '1': 'first_sequence_number',
      '3': 3,
      '4': 1,
      '5': 4,
      '10': 'firstSequenceNumber'
    },
    {
      '1': 'last_sequence_number',
      '3': 4,
      '4': 1,
      '5': 4,
      '10': 'lastSequenceNumber'
    },
    {'1': 'sequence_gaps', '3': 5, '4': 1, '5': 4, '10': 'sequenceGaps'},
  ],
};

/// Descriptor for `StreamSummary`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List streamSummaryDescriptor = $convert.base64Decode(
    'Cg1TdHJlYW1TdW1tYXJ5Eh8KC3Byb2R1Y2VyX2lkGAEgASgJUgpwcm9kdWNlcklkEicKD2FjY2'
    'VwdGVkX2V2ZW50cxgCIAEoBFIOYWNjZXB0ZWRFdmVudHMSMgoVZmlyc3Rfc2VxdWVuY2VfbnVt'
    'YmVyGAMgASgEUhNmaXJzdFNlcXVlbmNlTnVtYmVyEjAKFGxhc3Rfc2VxdWVuY2VfbnVtYmVyGA'
    'QgASgEUhJsYXN0U2VxdWVuY2VOdW1iZXISIwoNc2VxdWVuY2VfZ2FwcxgFIAEoBFIMc2VxdWVu'
    'Y2VHYXBz');
