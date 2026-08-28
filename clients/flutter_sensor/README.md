# TraceForge Sensor Client

This Flutter client is the real-device input adapter for TraceForge. It reads
accelerometer and gyroscope samples at the platform's game sampling interval
(requested at 20 ms), requests foreground GPS updates, and displays observed
rates and timestamps. It encodes samples with the shared versioned Protobuf
schema and streams them to the asynchronous C++ collector over gRPC.

## Run on a phone

Flutter and the appropriate platform toolchain are required. Connect a physical
device, then run:

```sh
flutter pub get
flutter devices
flutter run
```

Start the collector on the Mac:

```sh
./build/traceforge collect --listen 0.0.0.0:50051
```

Then launch the client. A simulator can use `127.0.0.1`; a physical phone must
use the Mac's LAN address:

```sh
flutter run \
  --dart-define=TRACEFORGE_COLLECTOR_HOST=MAC_LAN_IP \
  --dart-define=TRACEFORGE_COLLECTOR_PORT=50051 \
  --dart-define=TRACEFORGE_PRODUCER_ID=sebastians-iphone
```

Tap **Start** and grant location, motion, and local-network permissions.
Physical movement should update the motion values and sample counters. GPS may
update much more slowly and may be unreliable indoors. This development path
uses insecure local-network gRPC; it is not intended for internet exposure.

The client reports **Connecting** until the gRPC channel reaches its ready
state; it does not label a stream active merely because a channel object was
created. Connection attempts and collector acknowledgements are bounded by
five-second timeouts so an unreachable collector cannot leave Start or Stop
waiting indefinitely.

Before a phone is available, verify the complete Dart-to-C++ transport with
three deterministic sample events. Start the collector, then run in another
terminal:

```sh
dart run tool/send_sample_telemetry.dart --host 127.0.0.1 --port 50051
```

This checks cross-language serialization and the live RPC path. It does not
count as physical-sensor validation.

## Timestamp semantics

- Motion source timestamps are supplied by the platform sensor API and mapped
  by `sensors_plus` to Unix-epoch microseconds before TraceForge converts them
  to nanoseconds.
- GPS source timestamps are supplied by the location provider.
- Arrival timestamps are captured by the adapter using the phone wall clock.
- The C++ collector independently captures its wall-clock arrival timestamp.
- Source and collector clocks must not be subtracted for a latency claim until
  an explicit clock-offset estimate is implemented.

## Regenerate the Dart protocol bindings

After changing `../../proto/traceforge/v1/telemetry.proto`:

```sh
dart pub global activate protoc_plugin
mkdir -p lib/generated
protoc -I ../../proto \
  --dart_out=grpc:lib/generated \
  --plugin=protoc-gen-dart="$HOME/.pub-cache/bin/protoc-gen-dart" \
  ../../proto/traceforge/v1/telemetry.proto
```

## Local verification

```sh
flutter analyze
flutter test
```

Unit tests use an injected telemetry source so controller behavior can be tested
without pretending that simulated events validate physical hardware. They also
verify sensor-to-Protobuf encoding and publisher lifecycle behavior. Real sensor
sampling rates and the complete phone-to-C++ path were subsequently verified on
an iPhone 16 Pro Max: the collector accepted 8,114 accelerometer, gyroscope, and
GPS events with zero sequence gaps. See
[`../../docs/demo.md`](../../docs/demo.md) for the privacy-safe result, exact
commands, recording validation, and deterministic replay output.
