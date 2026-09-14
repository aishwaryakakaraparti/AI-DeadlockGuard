#!/usr/bin/env bash
# scripts/run_scenario.sh — Phase 6/7
#
# Usage: ./scripts/run_scenario.sh <scenario> [--auto-resolve] [--timeout N]
#
# Scenarios:
#   circular_wait_ipc       Phase 6: two-process deadlock, monitor detects via FIFO
#   dining_philosophers     Phase 5: dining philosophers (deadlocks)
#   dining_philosophers_fixed Phase 5: fixed philosophers (no deadlock)
#   high_contention         Phase 5: high contention, no deadlock
#
# Options:
#   --auto-resolve    Pass --auto-resolve to the monitor (Phase 7 auto-SIGKILL)
#   --timeout N       Kill the monitor after N seconds (default: 30)
#
# The script:
#   1. Starts the monitor daemon in the background.
#   2. Runs the scenario binary.
#   3. Waits for the scenario to finish (or timeout).
#   4. Stops the monitor.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SCENARIO="${1:-circular_wait_ipc}"
AUTO_RESOLVE=""
TIMEOUT=30

shift || true
while [[ $# -gt 0 ]]; do
    case "$1" in
        --auto-resolve) AUTO_RESOLVE="--auto-resolve" ;;
        --timeout)      TIMEOUT="$2"; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
done

MONITOR_BIN="$ROOT/monitor/build/monitor"
SCENARIO_BIN="$ROOT/harness/build/$SCENARIO"

# Check binaries exist
if [[ ! -x "$MONITOR_BIN" ]]; then
    echo "[run_scenario] monitor binary not found — run 'make' first"
    exit 1
fi
if [[ ! -x "$SCENARIO_BIN" ]]; then
    echo "[run_scenario] scenario binary '$SCENARIO' not found — run 'make' first"
    exit 1
fi

# Ensure log/status directories exist
mkdir -p "$ROOT/logs" "$ROOT/status"

echo "[run_scenario] Starting monitor (auto-resolve=$( [[ -n "$AUTO_RESOLVE" ]] && echo ON || echo OFF ))..."
"$MONITOR_BIN" $AUTO_RESOLVE \
    --features-path "$ROOT/status/features.json" \
    2>>"$ROOT/logs/monitor_events.log" &
MONITOR_PID=$!
echo "[run_scenario] Monitor PID=$MONITOR_PID"

# Give monitor time to open FIFO
sleep 0.3

echo "[run_scenario] Running scenario: $SCENARIO"
"$SCENARIO_BIN" 2>&1 &
SCENARIO_PID=$!

# Wait up to TIMEOUT seconds for the scenario to finish
ELAPSED=0
while kill -0 "$SCENARIO_PID" 2>/dev/null; do
    sleep 1
    ELAPSED=$((ELAPSED + 1))
    if [[ $ELAPSED -ge $TIMEOUT ]]; then
        echo "[run_scenario] Timeout after ${TIMEOUT}s — killing scenario"
        kill -9 "$SCENARIO_PID" 2>/dev/null || true
        break
    fi
done

# Give monitor a moment to write final state
sleep 0.5

echo "[run_scenario] Stopping monitor..."
kill "$MONITOR_PID" 2>/dev/null || true
wait "$MONITOR_PID" 2>/dev/null || true

echo "[run_scenario] Done. Log: $ROOT/logs/monitor_events.log"
