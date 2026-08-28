# Telemetry analysis contract

`traceforge analyze FILE` validates the entire `.tflog` before calculating any
metrics. A truncated or corrupt recording fails analysis; use `traceforge
recover` first only when `inspect` identifies an incomplete final record.

The report never prints sensor payload values such as coordinates or motion
vectors. It does print producer identifiers because they define the streams
being measured.

## Producer integrity

Records are examined in append order. Sequence metrics are grouped by producer:

- `sequence_gaps` sums missing values between increasing sequence numbers.
- `non_increasing_sequences` counts duplicate or decreasing numbers.

Sequence numbers belong to a producer, not an individual payload. This matters
for the phone client, which uses one sequence across its interleaved
accelerometer, gyroscope, and GPS samples.

## Stream timing

A stream is the combination of producer ID and payload type. Collector-arrival
timestamps are sorted within each stream before timing calculations:

- `rate_hz` is `(record count - 1) / (last arrival - first arrival)`.
- `interarrival_p50_ms` is the nearest-rank median between adjacent arrivals.
- `jitter_p95_ms` is the nearest-rank p95 absolute deviation of each interval
  from the median interval.

Rate and interval metrics are `n/a` when a stream has fewer than two samples;
rate is also `n/a` when its measured span is zero. A long pause, network batch,
or sensor stall remains visible rather than being discarded as an outlier.

`source_timestamp_regressions` counts timestamps that are less than or equal to
the preceding timestamp for the same producer, payload, and source clock domain
in append order. Source timestamps from different clock domains are never
compared.

## UTC arrival delta

For `CLOCK_DOMAIN_UTC` records, TraceForge calculates:

```text
collector arrival timestamp - source timestamp
```

The report provides p50, p95, p99, and maximum values in milliseconds using
nearest-rank percentiles. Negative values are valid. This metric includes clock
offset between the device and collector, sensor/API batching, scheduling, and
transport time. It is a cross-clock diagnostic and must not be presented as
pure network or ingestion latency unless the clocks were synchronized and their
error was measured separately. Non-UTC records are excluded.

## Resource bound

Analysis uses the same one-million-record default limit as inspection and
replay indexing. The parser validates record framing, contiguous indexes,
CRC32C checksums, Protobuf decoding, and schema version before returning a
complete report.
