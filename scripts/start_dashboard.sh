#!/usr/bin/env bash
# scripts/start_dashboard.sh
# One-command launcher for the full AI-DeadlockGuard system on Linux/WSL.
# Usage: bash scripts/start_dashboard.sh [--demo]

set -e
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

demo_mode=false
[[ "$1" == "--demo" ]] && demo_mode=true

echo "════════════════════════════════════════════════"
echo "  AI-DeadlockGuard — System Launcher"
echo "════════════════════════════════════════════════"

# Ensure status + logs dirs exist
mkdir -p "$ROOT/status" "$ROOT/logs"

# 1. Start monitor daemon (Linux only)
if ! $demo_mode; then
  echo "[1/3] Starting monitor daemon..."
  "$ROOT/monitor/build/monitor" > "$ROOT/logs/monitor.log" 2>&1 &
  MONITOR_PID=$!
  echo "      Monitor PID: $MONITOR_PID"
  sleep 0.5

  # 2. Start ML live predictor
  echo "[2/3] Starting live_predict.py..."
  python3 "$ROOT/predictor/live_predict.py" > "$ROOT/logs/predictor.log" 2>&1 &
  PRED_PID=$!
  echo "      Predictor PID: $PRED_PID"
fi

# 3. Start Flask dashboard
echo "[3/3] Starting Flask dashboard..."
python3 "$ROOT/dashboard/app.py" &
FLASK_PID=$!
echo "      Flask PID: $FLASK_PID"
echo ""
echo "  Dashboard → http://localhost:5000"
echo ""
echo "  Press Ctrl+C to stop all services."
echo "════════════════════════════════════════════════"

# Graceful shutdown on Ctrl+C
trap 'echo "Stopping..."; kill $FLASK_PID 2>/dev/null; kill $PRED_PID 2>/dev/null; kill $MONITOR_PID 2>/dev/null; exit 0' INT

wait $FLASK_PID
