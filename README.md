# TraceForge

TraceForge is a C++20 multi-sensor telemetry recorder and deterministic replay
engine. The project explores the systems problems behind ingesting concurrent
sensor streams, preserving timing and ordering information, recovering from
partial writes, and reproducing a recorded session reliably.

## Status

TraceForge currently provides a strict C++20 build, a minimal command-line
executable, Linux continuous integration, and a Flutter feasibility client for
reading real phone motion and location sensors. Network ingestion and recording
are not implemented yet. Physical-device results will be documented only after
the client is run on hardware.

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

The phone client is under `clients/flutter_sensor`:

```sh
cd clients/flutter_sensor
flutter pub get
flutter analyze
flutter test
flutter run
```

## Design direction

The core recorder and replay engine will remain in C++. Phone and web clients
will act only as data sources and debugging interfaces. Performance claims will
be added only after they are measured with documented, reproducible commands.
