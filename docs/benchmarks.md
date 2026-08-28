# Reproducible performance measurements

These are measured results, not capacity guarantees. The benchmark uses the
real loopback gRPC path and reports machine-readable output so results can be
compared without editing the program.

## Environment

- Date: August 26, 2026
- Hardware: MacBook Pro (`Mac16,8`), Apple M4 Pro
- CPU: 12 cores (8 performance, 4 efficiency)
- Memory: 24 GB
- Operating system: macOS 26.6 (`25G72`)
- Compiler: Apple Clang 21.0.0
- Build: CMake 4.4.3, `Release`
- Protobuf compiler: 36.0
- Workload: 250,000 camera-metadata events across seven concurrent producers
- Bounded queue capacity: 65,536 events
- Sample count: five independent process runs

Build and run the exact workload:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DTRACEFORGE_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
for run in $(seq 1 5); do
  ./build/traceforge_benchmark \
    --events 250000 \
    --producers 7 \
    --queue-capacity 65536
done
```

The benchmark writes its temporary recording to the operating system's temp
directory and removes it on exit. Recording creation is intentionally excluded
from replay timing. Indexing begins immediately after file creation, so the
19.81 MiB file is normally in the local filesystem cache.

## Results

The table reports the median of five runs. Ranges show the minimum and maximum
observed values from the same five-run sample.

| Measurement | Median | Five-run range |
| --- | ---: | ---: |
| Loopback ingestion throughput | 103,715 events/s | 99,136–110,183 |
| Collector queue latency p50 | 12 µs | 11–12 |
| Collector queue latency p95 | 43 µs | 33–44 |
| Collector queue latency p99 | 81 µs | 50–98 |
| Ingestion process CPU utilization | 650.62% | 625.77–703.15% |
| Queue high-water mark | 68 events | 28–98 |
| Ingestion rejected events | 0 | 0–0 |
| Ingestion peak resident memory | 79.78 MiB | 79.62–80.03 |
| Recording index throughput | 1,626,049 records/s | 1,615,577–1,647,616 |
| Immediate replay throughput | 1,684,174 records/s | 1,667,600–1,729,572 |
| Replay peak resident memory | 89.67 MiB | 89.52–89.91 |

CPU utilization is process CPU time divided by wall time. It may exceed 100%
because the client producers, gRPC runtime, server callbacks, and queue worker
run concurrently across cores.

Queue latency begins when the collector callback timestamps a validated event
and ends when the queue consumer receives it. It is not client-to-server
network latency. Throughput includes client grouping, Protobuf serialization,
seven concurrent loopback streams, collector validation, queueing, and consumer
drain. Peak resident memory includes the benchmark's input event vector, gRPC,
the queue, the replay index, and other process state.

## Profile-guided replay change

The initial implementation called `seekg` for every replayed record, including
the common case where collector-time order and physical append order were
identical. Before optimization, the five-run median was 487,712 records/s
(483,051–497,267 records/s).

Replay now tracks the next physical record offset. It skips the fixed-size
header sequentially when the next indexed record is physically adjacent and
falls back to an absolute seek for genuinely out-of-order records. The same
tests still cover out-of-order offsets, and every before/after run produced the
same deterministic hash, `d6977ce5f0c788d2`.

The median improved from 487,712 to 1,684,174 records/s: a 3.45x speedup on this
workload. This optimization changes neither file format nor replay ordering.

### Independent before/after reproduction

The comparison was repeated on August 28, 2026 from source commit
`20291373ded8f355a3ab4e45da8ee022e2db7705`. Two detached worktrees used the
same compiler, Release configuration, benchmark, and command. The baseline
worktree applied only
[`benchmarks/replay-always-seek-baseline.patch`](../benchmarks/replay-always-seek-baseline.patch),
which restores an unconditional `seekg` for each record. Five runs of each
variant were interleaved to reduce ordering bias.

| Variant | Median replay rate | Five-run range |
| --- | ---: | ---: |
| Unconditional-seek baseline | 480,101 records/s | 432,868-488,302 |
| Sequential-read optimization | 1,685,066 records/s | 1,601,724-1,721,612 |

The reproduced median speedup is **3.51x**. All ten runs replayed 250,000
records and produced the same hash, `d6977ce5f0c788d2`. The complete
machine-readable outputs are preserved for the
[`baseline`](../benchmarks/results/2026-08-28-replay-baseline.txt) and
[`optimized`](../benchmarks/results/2026-08-28-replay-optimized.txt) variants.
Together with the original 3.45x measurement, the repeated comparison supports
the conservative claim that the fast path improved this workload by about
3.5x.

## Interpreting the numbers

These measurements characterize one laptop, build, dependency set, payload,
and loopback workload. They should be rerun after material code or dependency
changes. Physical networking, storage durability settings, larger payloads,
and a real downstream consumer will produce different results.
