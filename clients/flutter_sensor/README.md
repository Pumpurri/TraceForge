# TraceForge Sensor Client

This Flutter client is the real-device input adapter for TraceForge. It reads
accelerometer and gyroscope samples at the platform's game sampling interval
(requested at 20 ms), requests foreground GPS updates, and displays observed
rates and timestamps before any networking is introduced.

## Run on a phone

Flutter and the appropriate platform toolchain are required. Connect a physical
device, then run:

```sh
flutter pub get
flutter devices
flutter run
```

Tap **Start** and grant location and motion permissions. Physical movement
should update the motion values and sample counters. GPS may update much more
slowly and may be unreliable indoors.

## Timestamp semantics

- Motion source timestamps are supplied by the platform sensor API.
- GPS source timestamps are supplied by the location provider.
- Arrival timestamps are captured by the adapter using the phone wall clock.
- These values are not directly comparable to the future desktop collector's
  clock. Cross-device latency will require explicit clock-offset estimation.

## Local verification

```sh
flutter analyze
flutter test
```

Unit tests use an injected telemetry source so controller behavior can be tested
without pretending that simulated events validate physical hardware. Real
sensor sampling rates and device-specific behavior must be measured on a phone.
