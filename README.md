# TraceForge

TraceForge is a C++20 multi-sensor telemetry recorder and deterministic replay
engine. The project explores the systems problems behind ingesting concurrent
sensor streams, preserving timing and ordering information, recovering from
partial writes, and reproducing a recorded session reliably.

## Status

TraceForge currently provides a strict C++20 build, Linux continuous
integration, a versioned Protobuf telemetry contract, an asynchronous C++ gRPC
collector, a deterministic multi-sensor workload generator, and a Flutter
client for reading and streaming real phone motion and location sensors.
Persistent recording is not implemented yet. Physical-device results will be
documented only after the client is run on hardware.

## Planned capabilities

- Stream real accelerometer, gyroscope, and GPS data from a phone.
- Ingest multiple concurrent producers through an asynchronous C++ service.
- Apply bounded buffering and explicit backpressure behavior.
- Persist events in a versioned, checksummed append-only format.
- Recover complete records after an interrupted write.
- Seek and deterministically replay synchronized telemetry.
- Test malformed inputs, concurrency, and failure recovery with fuzzers and
  sanitizers.
- Publish reproducible latency, throughput, and memory benchmarks.

## Build

Requirements:

- A C++20 compiler
- CMake 3.20 or newer
- Protobuf and its compiler
- gRPC C++ and the gRPC Protobuf compiler plugin

On macOS with Homebrew:

```sh
brew install cmake protobuf grpc
```

On Ubuntu:

```sh
sudo apt-get install libgrpc++-dev libprotobuf-dev \
  protobuf-compiler protobuf-compiler-grpc
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run the initial CLI:

```sh
./build/traceforge --help
./build/traceforge --version
```

Run the asynchronous collector on the default insecure development endpoint:

```sh
./build/traceforge collect --listen 0.0.0.0:50051
```

The ingestion protocol is defined in
`proto/traceforge/v1/telemetry.proto`. Every event carries a schema version,
producer identity, monotonically increasing sequence number, source timestamp,
explicit clock domain, and typed payload. The collector validates events,
captures its own arrival timestamp, reports sequence gaps, and returns explicit
gRPC errors for invalid input or an overloaded sink.

Generate one simulated second of telemetry and print its reproducibility hash:

```sh
./build/traceforge generate --seed 42 --duration-ms 1000
```

The default workload models a combined 100 Hz IMU stream (50 Hz accelerometer
and 50 Hz gyroscope), 30 Hz camera metadata, 10 Hz GPS, 1 Hz temperature, 2 Hz
fault monitoring, and asynchronous controller state transitions. Generation is
deterministic for a seed and supports portable integer-based timestamp jitter,
clock drift, drops, duplicates, and adjacent-event reordering:

```sh
./build/traceforge generate \
  --seed 8675309 \
  --duration-ms 5000 \
  --jitter-us 200 \
  --clock-drift-ppm 125 \
  --drop-per-million 1000 \
  --duplicate-per-million 500 \
  --reorder-per-million 500
```

Use `--print-events` to inspect the ordered event stream. Rates, fault counts,
and the hash are always reported; the hash is for reproducibility checks, not
cryptographic integrity.

The phone client is under `clients/flutter_sensor`:

```sh
cd clients/flutter_sensor
flutter pub get
flutter analyze
flutter test
flutter run
```

To stream from a physical phone, pass the collector Mac's LAN address through
`--dart-define=TRACEFORGE_COLLECTOR_HOST=...`; `127.0.0.1` is only the default
for a simulator running on the same Mac. See the client README for the complete
command and platform permissions.

## Design direction

The core recorder and replay engine will remain in C++. Phone and web clients
will act only as data sources and debugging interfaces. Performance claims will
be added only after they are measured with documented, reproducible commands.
