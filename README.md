# TraceForge

TraceForge is a C++20 multi-sensor telemetry recorder and deterministic replay
engine. The project explores the systems problems behind ingesting concurrent
sensor streams, preserving timing and ordering information, recovering from
partial writes, and reproducing a recorded session reliably.

## Status

TraceForge currently provides a strict C++20 build, Linux continuous
integration, a versioned Protobuf telemetry contract, an asynchronous C++ gRPC
collector with bounded ingestion, a deterministic multi-sensor workload
generator, and a Flutter client for reading and streaming real phone motion and
location sensors. Accepted events can be persisted in a versioned, checksummed
append-only log with interrupted-tail recovery. Physical-device results will be
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

### Bounded ingestion and backpressure

gRPC callback threads never perform consumer work directly. Accepted events
move into a fixed-capacity FIFO, and one worker drains that queue into the next
pipeline stage. The queue tracks accepted and rejected events, current depth,
and its high-water mark.

The overload policy is explicit: TraceForge does not block an asynchronous RPC
thread and does not silently drop an event. If the queue is full, the affected
producer stream ends with gRPC `RESOURCE_EXHAUSTED`; other streams remain
independent. A stopped or failed pipeline returns `UNAVAILABLE`.

Queue capacity is configurable. An artificial consumer delay makes overload
behavior reproducible during development:

```sh
./build/traceforge collect \
  --listen 0.0.0.0:50051 \
  --queue-capacity 64 \
  --consumer-delay-us 5000
```

Automated tests exercise FIFO draining, fixed memory bounds, concurrent queue
producers, six simultaneous gRPC streams, producer reconnection, and overload
status propagation.

The collector handles `SIGINT` and `SIGTERM` as graceful shutdown requests. It
stops gRPC, drains accepted queue entries, flushes the recording, and reports
final accepted, rejected, consumed, high-watermark, and consumer-failure
counters before exiting.

### Binary recording and recovery

Pass an output file to persist the queue consumer's accepted events:

```sh
./build/traceforge collect \
  --listen 0.0.0.0:50051 \
  --output session.tflog \
  --flush-every 64
```

Inspect the file header, every record header, CRC32C checksums, declared payload
lengths, contiguous record indexes, and Protobuf payloads:

```sh
./build/traceforge inspect session.tflog
```

If a shutdown interrupts only the final record, recover all prior complete
records:

```sh
./build/traceforge recover session.tflog
```

Recovery refuses completed-record corruption; it never silently truncates past
a checksum error. The complete byte layout and durability semantics are in
[`docs/recording-format.md`](docs/recording-format.md).

### Indexed deterministic replay

Open a validated recording, build its timestamp-to-file-offset index, and
replay immediately:

```sh
./build/traceforge replay session.tflog
```

Replay uses collector-arrival timestamps because they share one clock domain
across all producers. It orders records by that timestamp, then by append index
when timestamps are equal. Source timestamps remain available to consumers but
are not compared across incompatible device clock domains.

Seek to an inclusive collector timestamp, stop before an exclusive timestamp,
and reproduce the original timing at 2x speed:

```sh
./build/traceforge replay session.tflog \
  --from-ns 1700000000000000000 \
  --until-ns 1700000005000000000 \
  --speed 2 \
  --print-events
```

Speed `0` is the default and replays without sleeping. Every run reports a
stable 64-bit hash over the selected ordered records, their collector
timestamps, and their exact recorded Protobuf bytes. The index holds metadata
and file offsets rather than retaining every decoded event; replay seeks to
each payload and verifies its CRC32C before delivery. See
[`docs/replay.md`](docs/replay.md) for the precise window, ordering, pacing, and
hash contracts.

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

With a collector running, stream the workload over seven concurrent gRPC
streams—one for each virtual producer:

```sh
./build/traceforge generate \
  --seed 42 \
  --duration-ms 1000 \
  --target 127.0.0.1:50051
```

The command reports attempted, written, and collector-accepted counts plus the
gRPC status for every producer. It exits unsuccessfully if any stream is
rejected, including intentional validation faults or queue overload. This makes
the same seeded workload reusable for normal ingestion, reconnect, and
backpressure tests.

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

## Reliability tooling

TraceForge provides opt-in CMake configurations for AddressSanitizer,
UndefinedBehaviorSanitizer, and ThreadSanitizer, plus a Clang libFuzzer target
that tests both arbitrary bytes and structured mutations of valid recordings.
The exact local and CI commands, instrumentation boundary, and documented gRPC
TSan exclusion are in [`docs/testing.md`](docs/testing.md).
