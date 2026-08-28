#!/usr/bin/env bash

set -eu

TRACEFORGE_BINARY=$1
TRACEFORGE_TEST_ROOT=${TMPDIR:-/tmp}
TRACEFORGE_TEST_DIR=$(mktemp -d "$TRACEFORGE_TEST_ROOT/traceforge_shutdown.XXXXXX")
TRACEFORGE_OUTPUT="$TRACEFORGE_TEST_DIR/collector.out"
TRACEFORGE_LOG="$TRACEFORGE_TEST_DIR/session.tflog"
TRACEFORGE_PID=

cleanup() {
    if [ -n "$TRACEFORGE_PID" ] && kill -0 "$TRACEFORGE_PID" 2>/dev/null; then
        kill -KILL "$TRACEFORGE_PID" 2>/dev/null || true
        wait "$TRACEFORGE_PID" 2>/dev/null || true
    fi
    rm -f "$TRACEFORGE_OUTPUT" "$TRACEFORGE_LOG"
    rmdir "$TRACEFORGE_TEST_DIR" 2>/dev/null || true
}
trap cleanup EXIT

"$TRACEFORGE_BINARY" collect \
    --listen 127.0.0.1:0 \
    --output "$TRACEFORGE_LOG" \
    --flush-every 1 >"$TRACEFORGE_OUTPUT" 2>&1 &
TRACEFORGE_PID=$!

TRACEFORGE_STARTED=false
for TRACEFORGE_ATTEMPT in $(seq 1 100); do
    if grep -q "TraceForge collector listening" "$TRACEFORGE_OUTPUT"; then
        TRACEFORGE_STARTED=true
        break
    fi
    if ! kill -0 "$TRACEFORGE_PID" 2>/dev/null; then
        break
    fi
    sleep 0.05
done

if [ "$TRACEFORGE_STARTED" != true ]; then
    cat "$TRACEFORGE_OUTPUT"
    exit 1
fi

kill -INT "$TRACEFORGE_PID"
for TRACEFORGE_ATTEMPT in $(seq 1 100); do
    if ! kill -0 "$TRACEFORGE_PID" 2>/dev/null; then
        break
    fi
    sleep 0.05
done

if kill -0 "$TRACEFORGE_PID" 2>/dev/null; then
    exit 1
fi

wait "$TRACEFORGE_PID"
TRACEFORGE_PID=
grep -q "collector_shutdown=signal signal=2" "$TRACEFORGE_OUTPUT"
"$TRACEFORGE_BINARY" inspect "$TRACEFORGE_LOG"
"$TRACEFORGE_BINARY" analyze "$TRACEFORGE_LOG" |
    grep -q "status=complete records=0"
