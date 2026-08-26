# Deterministic replay contract

TraceForge builds a replay index when a `.tflog` is opened. The existing log
scanner validates the file header, every record header, declared payload size,
CRC32C checksum, contiguous append index, Protobuf encoding, and schema version.
It retains one compact index entry per record:

- append index;
- byte offset and payload length;
- payload CRC32C;
- collector-arrival timestamp; and
- source timestamp for post-index consistency checks.

Decoded Protobuf events are not retained by the index. During replay, the
engine seeks directly to each selected payload offset, reads only that payload,
checks its CRC32C again, and then decodes it. Index construction is a single
linear validation pass; selecting a starting timestamp uses binary search.

## Timeline and ordering

The replay timeline is the signed 64-bit collector-arrival timestamp in
nanoseconds. Collector timestamps use one host clock domain even when producers
report UTC, monotonic device time, or other source clocks. Using source time as
a global ordering key would incorrectly compare unrelated clock domains.

Records are ordered by:

1. collector-arrival timestamp ascending; then
2. append index ascending.

The append index is unique, so the ordering is total and stable for equal or
out-of-order timestamps. Replay never rewrites producer sequence numbers or
source timestamps.

`--from-ns` is inclusive and `--until-ns` is exclusive, forming the half-open
interval `[from, until)`. The lower bound is found in the sorted index rather
than by scanning records from the beginning.

## Pacing

`--speed 0` emits selected records as quickly as the consumer accepts them.
Positive speeds reproduce timestamp deltas against a monotonic wall clock:

- `--speed 1` uses recorded timing;
- `--speed 2` runs twice as fast; and
- `--speed 0.5` runs at half speed.

The first selected record establishes both the recording origin and wall-clock
origin. Equal timestamps are delivered consecutively without an artificial
delay. Pacing does not affect ordering or the replay hash.

## Replay hash

The reported 16-digit hexadecimal value is streaming FNV-1a over each selected
record in replay order. Each record contributes its append index, collector
timestamp, payload length, and exact recorded Protobuf payload bytes. Integer
fields are encoded little-endian before hashing.

The hash detects changes in the selected range, ordering, metadata, or payload.
It is intended for reproducibility tests, not security or file integrity;
CRC32C remains the format-level corruption check.

Replay refuses incomplete or corrupt logs. Recover a genuinely truncated final
record with `traceforge recover` before replaying; completed-record corruption
is never truncated automatically.
