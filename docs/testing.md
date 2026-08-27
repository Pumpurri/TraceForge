# Reliability testing

The default suite covers schema validation, Protobuf round trips,
deterministic generation, bounded queue behavior, concurrent gRPC ingestion,
producer reconnects, overload rejection, checksummed recording, recovery,
indexed replay, and process shutdown.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DTRACEFORGE_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The graceful-shutdown test launches the real collector as a subprocess, sends
`SIGINT`, requires a zero exit status, checks the final shutdown counters, and
validates the flushed recording.

## AddressSanitizer and UndefinedBehaviorSanitizer

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DTRACEFORGE_ENABLE_ASAN=ON \
  -DTRACEFORGE_ENABLE_UBSAN=ON
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

Sanitizer flags apply to TraceForge targets but not generated Protobuf code or
prebuilt dependency libraries. This keeps instrumentation on the code owned by
the project and avoids mixing instrumented generated implementations with an
uninstrumented shared Protobuf runtime.

## ThreadSanitizer

```sh
cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug \
  -DTRACEFORGE_ENABLE_TSAN=ON
cmake --build build-tsan --parallel
TSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-tsan \
  --output-on-failure \
  --exclude-regex \
  'traceforge\.(collector_integration|workload_publisher_integration)'
```

The TSan job exercises TraceForge's bounded queue, queue consumer, recorder,
replay engine, and graceful shutdown. The two callback-heavy network tests are
excluded from TSan because distro and Homebrew gRPC libraries are prebuilt
without TSan instrumentation and report inside gRPC callback internals. Those
tests still run in the complete default and ASan+UBSan suites. No global race
suppression is used.

## Recording parser fuzzer

The Clang libFuzzer target has two input modes. Odd first bytes treat the rest
of the input as a completely arbitrary file. Even first bytes treat subsequent
byte pairs as offset/XOR mutations against a valid checksummed recording. The
second mode reaches record headers, payloads, CRC checks, Protobuf decoding,
and recovery paths immediately instead of waiting for the fuzzer to guess a
valid 32-bit header checksum.

```sh
CC=clang CXX=clang++ cmake -S . -B build-fuzz \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=OFF \
  -DTRACEFORGE_BUILD_FUZZER=ON \
  -DTRACEFORGE_ENABLE_ASAN=ON \
  -DTRACEFORGE_ENABLE_UBSAN=ON
cmake --build build-fuzz --target traceforge_recording_fuzz
./build-fuzz/traceforge_recording_fuzz -runs=5000 -max_len=1048576
```

CI runs both sanitizer configurations and a bounded 5,000-input fuzz smoke
test on every push and pull request. Longer local fuzzing can replace `-runs`
with `-max_total_time=SECONDS`.
