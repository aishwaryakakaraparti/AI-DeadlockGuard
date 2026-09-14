# AI-DeadlockGuard

A Linux-based deadlock detection and AI early-warning prediction system with interactive live demo visualization.

## Overview
Deadlocks freeze systems silently. Standard tools (lockdep, eBPF, Valgrind, GDB) only notify after a deadlock has occurred. **AI-DeadlockGuard** combines two layers:
1. **Classical Detection (Guaranteed):** Intercepts lock/unlock operations across worker processes, maintains a live wait-for graph, detects cycles, and breaks deadlocks via signal-based intervention.
2. **AI Early Warning (Prediction):** A Random Forest model trained on simulated executions continuously scores deadlock risk based on blocked processes, wait-time growth, and graph density to intervene *before* a deadlock forms.

## Project Structure
- `harness/`: Worker process scenarios (circular wait, dining philosophers, high contention, delayed deadlock).
- `ipc/`: Inter-process communication protocol and named pipe (FIFO) messaging.
- `monitor/`: Central monitoring daemon implementing graph tracking, cycle detection, feature extraction, and signal resolution.
- `predictor/`: Machine learning data pipeline, dataset generation, model training, and live risk scoring engine.
- `scripts/`: Offline validation scripts and scenario launchers.
- `gui/`: Flask web backend and vanilla JavaScript real-time visual dashboard.
- `status/`: Runtime state files (`heartbeat.json`, `risk.json`, `events.log`).
- `logs/`: Execution logs.
- `tests/`: Automated unit and integration test suites.
- `docs/`: System design docs and syllabus mapping reports.
