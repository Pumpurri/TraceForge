# TraceForge Recording Format v1

TraceForge recordings use an append-only, little-endian container. The format
is deliberately small: Protobuf defines event payloads, while the container
adds framing, ordering, integrity checks, and interrupted-write recovery.

## File header

The file begins with a fixed 32-byte header:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 8 | Magic: `TFLOG\r\n\x1a` |
| 8 | 2 | Format version (`1`) |
| 10 | 2 | Header size (`32`) |
| 12 | 4 | Flags; reserved as zero |
| 16 | 8 | Creation time in Unix nanoseconds |
| 24 | 4 | Reserved as zero |
| 28 | 4 | CRC32C of bytes 0–27 |

Readers reject unknown versions, sizes, magic values, or header checksums.

## Records

Each record consists of a fixed 36-byte header followed immediately by one
serialized `traceforge.v1.TelemetryEvent` payload:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | Record magic: `TFR1` |
| 4 | 2 | Record-header size (`36`) |
| 6 | 2 | Record version (`1`) |
| 8 | 4 | Payload length in bytes |
| 12 | 4 | CRC32C of the payload |
| 16 | 8 | Contiguous record index, starting at zero |
| 24 | 8 | Collector arrival time in Unix nanoseconds |
| 32 | 4 | CRC32C of record-header bytes 0–31 |

Payloads are limited to 4 MiB. Readers validate the record header and declared
length before allocating payload memory, then validate the payload checksum and
Protobuf schema version before exposing the record.

CRC32C detects accidental corruption; it is not authentication and does not
protect against deliberate modification.

## Durability and recovery

The writer appends a complete record header followed by its payload. The flush
interval is explicit and configurable; a successful append does not imply an
`fsync` or durable-storage guarantee.

Writers create a new file and refuse to overwrite an existing path.

After interruption, a reader retains every fully checksummed record before an
incomplete final header or payload. The `recover` command truncates only that
incomplete tail. It refuses to modify files containing a bad checksum, invalid
magic, unsupported version, noncontiguous record index, or other completed-data
corruption.
