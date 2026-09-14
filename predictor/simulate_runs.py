#!/usr/bin/env python3
"""
simulate_runs.py — Phase 8

Repeatedly runs harness scenarios through the monitor to collect live feature
snapshots and generate a labeled training dataset for the ML models.
Run this script inside a Linux/WSL environment where the C binaries are compiled.
"""

import os
import time
import json
import subprocess
import pandas as pd
import random

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
MONITOR_BIN = os.path.join(ROOT_DIR, 'monitor', 'build', 'monitor')
HARNESS_DIR = os.path.join(ROOT_DIR, 'harness', 'build')
FEATURES_PATH = os.path.join(ROOT_DIR, 'status', 'features.json')
HEARTBEAT_PATH = os.path.join(ROOT_DIR, 'status', 'heartbeat.json')
CSV_OUT = os.path.join(ROOT_DIR, 'predictor', 'data', 'training_data.csv')

SCENARIOS = {
    'circular_wait_ipc': {'runs': 30, 'expected_deadlock': True},
    'dining_philosophers': {'runs': 30, 'expected_deadlock': True},
    'dining_philosophers_fixed': {'runs': 30, 'expected_deadlock': False},
    'high_contention': {'runs': 30, 'expected_deadlock': False},
}

def clear_status_files():
    for p in [FEATURES_PATH, HEARTBEAT_PATH]:
        if os.path.exists(p):
            os.remove(p)

def read_json(path):
    if not os.path.exists(path):
        return None
    try:
        with open(path, 'r') as f:
            return json.load(f)
    except:
        return None

def run_simulation():
    all_data = []

    for scenario, config in SCENARIOS.items():
        scenario_bin = os.path.join(HARNESS_DIR, scenario)
        if not os.path.exists(scenario_bin):
            print(f"Skipping {scenario} — binary not found (run 'make' first).")
            continue

        print(f"--- Running scenario: {scenario} ({config['runs']} runs) ---")
        
        for run_idx in range(config['runs']):
            clear_status_files()
            
            # Start monitor
            monitor_proc = subprocess.Popen(
                [MONITOR_BIN, '--features-path', FEATURES_PATH],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )
            time.sleep(0.3) # Wait for FIFO to open
            
            # Start scenario
            scenario_proc = subprocess.Popen(
                [scenario_bin],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )

            snapshots = []
            deadlock_occurred = False
            start_time = time.time()
            
            # Poll for features and heartbeat
            while True:
                time.sleep(0.1)
                
                features = read_json(FEATURES_PATH)
                if features and (not snapshots or snapshots[-1] != features):
                    snapshots.append(features)

                heartbeat = read_json(HEARTBEAT_PATH)
                if heartbeat and heartbeat.get('status') == 'deadlock':
                    deadlock_occurred = True
                    break
                
                # Check if scenario finished (clean exit)
                if scenario_proc.poll() is not None:
                    break
                    
                # Timeout safeguard
                if time.time() - start_time > 10:
                    break

            # Terminate processes
            if scenario_proc.poll() is None:
                scenario_proc.kill()
            monitor_proc.kill()
            monitor_proc.wait()

            # Labeling strategy:
            # If a deadlock occurred, the snapshots leading up to it are highly predictive.
            # We label the last few snapshots as 1 (imminent deadlock), others as 0.
            # If no deadlock, all snapshots are 0.
            for i, snap in enumerate(snapshots):
                # Mark as deadlock if deadlock occurred and it's in the later half of the run
                is_deadlock = 1 if (deadlock_occurred and i >= len(snapshots) / 2) else 0
                
                all_data.append({
                    'blocked_count': snap.get('blocked_count', 0),
                    'wait_time_growth': snap.get('wait_time_growth', 0.0),
                    'edge_count': snap.get('edge_count', 0),
                    'graph_density': snap.get('graph_density', 0.0),
                    'is_deadlock_imminent': is_deadlock
                })
            
            print(f"  Run {run_idx+1}/{config['runs']} -> Snapshots: {len(snapshots)}, Deadlock: {deadlock_occurred}")

    # Save to CSV
    os.makedirs(os.path.dirname(CSV_OUT), exist_ok=True)
    df = pd.DataFrame(all_data)
    
    # Simple jitter to prevent duplicate exact rows from dominating, helping tree models generalize better
    df['wait_time_growth'] += [random.uniform(-0.01, 0.01) for _ in range(len(df))]
    
    df.to_csv(CSV_OUT, index=False)
    print(f"\nSaved {len(df)} feature snapshots to {CSV_OUT}")

if __name__ == "__main__":
    run_simulation()
