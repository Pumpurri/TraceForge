# Physical sensor demonstration

This walkthrough demonstrates the complete TraceForge path without requiring a
reader to inspect the implementation:

```text
physical iPhone sensors -> Protobuf -> gRPC over Wi-Fi -> C++ bounded queue
-> checksummed recording -> validation, recovery, and deterministic replay
```

It is designed as a 90-second screen recording, but the commands also serve as
a reproducible manual acceptance test.

## What the demonstration proves

- Motion and GPS samples originate from physical phone APIs, not fixtures.
- The Dart and C++ processes interoperate through the shared Protobuf schema.
- The asynchronous collector accepts the stream through its bounded queue and
  returns an exact accepted-event and sequence-gap summary.
- The finalized recording is structurally valid, checksum-valid, and readable.
- Replay produces the same record count and stable hash at different speeds.
- An incomplete final record can be detected and removed without hiding
  corruption in any completed record.

## Prerequisites

- Build TraceForge in `Release` or `Debug` mode.
- Pair a physical iPhone with Xcode and enable Developer Mode.
- Install Flutter dependencies under `clients/flutter_sensor`.
- Put the phone and collector on the same non-isolated Wi-Fi LAN.
- Grant TraceForge Sensor access to Local Network, Motion & Fitness, and
  foreground Location.

Find the Mac's current LAN address immediately before launching the client:

```sh
ipconfig getifaddr en0
```

Do not use `127.0.0.1` for a physical phone. Do not publish device identifiers,
local IP addresses, Apple signing information, or sensor payload values in a
public recording.

## 1. Record physical telemetry

In terminal A, start the C++ collector with per-record flushing for a compact
demonstration:

```sh
./build/traceforge collect \
  --listen 0.0.0.0:50051 \
  --output physical-session.tflog \
  --flush-every 1
```

In terminal B, launch the signed client. Replace both placeholders with values
reported by `flutter devices` and `ipconfig getifaddr en0`:

```sh
cd clients/flutter_sensor
flutter run -d PHYSICAL_DEVICE_ID \
  --dart-define=TRACEFORGE_COLLECTOR_HOST=MAC_LAN_IP \
  --dart-define=TRACEFORGE_COLLECTOR_PORT=50051 \
  --dart-define=TRACEFORGE_PRODUCER_ID=demo-iphone
```

The UI must show **Connecting** until gRPC reaches its ready state, followed by
**Streaming**. Tap **Start**, move and rotate the phone for 15-30 seconds, and
then tap **Stop**. The phone should report the collector-accepted count and zero
sequence gaps. Press `Ctrl-C` in terminal A only after the phone has stopped its
stream, allowing the collector to drain and flush its queue.

## 2. Validate and replay

```sh
./build/traceforge inspect physical-session.tflog
./build/traceforge replay physical-session.tflog
./build/traceforge replay physical-session.tflog --speed 1000
```

Immediate and paced runs must report identical indexed/replayed counts and the
same hash. `--speed 1000` keeps the recording short enough for a demo while
still exercising the pacing path.

## 3. Demonstrate interrupted-tail recovery

Never damage the original recording. Truncate a copy, inspect the detected
failure, recover its complete prefix, and inspect it again:

```sh
cp physical-session.tflog interrupted-copy.tflog
truncate -s -7 interrupted-copy.tflog
./build/traceforge inspect interrupted-copy.tflog
./build/traceforge recover interrupted-copy.tflog
./build/traceforge inspect interrupted-copy.tflog
```

The first `inspect` intentionally exits unsuccessfully because the final payload
is incomplete. `recover` truncates only to the last verified record boundary.

## Verified reference output

The reference run was captured on August 27, 2026 with an iPhone 16 Pro Max and
an Apple M4 Pro Mac. It lasted 80.5 seconds and produced 4,054 accelerometer,
4,053 gyroscope, and 7 GPS samples. GPS coordinates and device/network
identifiers are intentionally excluded.

The following blocks retain the fields needed to verify behavior while omitting
absolute timestamps and other machine-specific details:

```text
Collector accepted 8114 events; 0 sequence gaps
collector_shutdown=signal signal=2 accepted=8114 rejected=0 consumed=8114 queue_high_watermark=21 consumer_failures=0

path=physical-session.tflog status=complete records=8114 file_bytes=819416 valid_bytes=819416
path=physical-session.tflog status=complete indexed=8114 replayed=8114 speed=0 hash=ac5b42e5a8547b48
path=physical-session.tflog status=complete indexed=8114 replayed=8114 speed=1000 hash=ac5b42e5a8547b48
```

The recovery path was verified by removing seven bytes from a copy:

```text
path=interrupted-copy.tflog status=truncated_tail records=8113 file_bytes=819409 valid_bytes=819315 error_offset=819315 message="final record payload is incomplete"
path=interrupted-copy.tflog result=success changed=true original_bytes=819409 recovered_bytes=819315 message="removed incomplete final record"
path=interrupted-copy.tflog status=complete records=8113 file_bytes=819315 valid_bytes=819315
```

## 90-second recording outline

1. **0:00-0:10 — Problem and architecture.** Show the README diagram and state
   that TraceForge records concurrent sensor streams for deterministic debugging.
2. **0:10-0:25 — Start the real path.** Show the collector listening, launch the
   physical iPhone client, and wait for **Streaming**.
3. **0:25-0:45 — Capture.** Move the phone so accelerometer and gyroscope values
   change; briefly show the sample rates. Keep GPS coordinates cropped out.
4. **0:45-0:55 — Close the stream.** Tap **Stop** and show the accepted-event and
   zero-gap acknowledgement, then stop the collector to display its counters.
5. **0:55-1:10 — Validate and replay.** Run `inspect`, immediate replay, and
   1000x replay; point out the identical count and hash.
6. **1:10-1:25 — Recover.** Truncate a copy, show explicit tail detection, run
   `recover`, and re-inspect the verified prefix.
7. **1:25-1:30 — Close.** Show the measured benchmark line and link to the
   sanitizer/fuzzing documentation.

## Recording checklist

- Use a terminal font large enough to read at 1080p.
- Crop or blur GPS coordinates, local IP addresses, device IDs, and Apple team
  information.
- Keep the original `.tflog` out of Git because it contains real sensor data.
- Do not call queue latency end-to-end latency; phone and collector clocks are
  not synchronized.
- Record one continuous take, then trim pauses rather than hiding failed output.
