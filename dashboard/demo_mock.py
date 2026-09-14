#!/usr/bin/env python3
"""
demo_mock.py — Demo mode data generator

Writes mock features.json, heartbeat.json, risk.json, and monitor_events.log
to the status/ and logs/ directories so the dashboard looks fully alive even
when the C monitor and live_predict.py are not running.

Run this in a separate terminal alongside app.py for Windows demos:
    python dashboard/demo_mock.py
"""

import os
import json
import time
import math
import random
from datetime import datetime

ROOT_DIR      = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
FEATURES_PATH = os.path.join(ROOT_DIR, 'status', 'features.json')
HEARTBEAT_PATH= os.path.join(ROOT_DIR, 'status', 'heartbeat.json')
RISK_PATH     = os.path.join(ROOT_DIR, 'status', 'risk.json')
LOG_PATH      = os.path.join(ROOT_DIR, 'logs', 'monitor_events.log')

MAX_NODES = 64
MAX_EDGES = MAX_NODES * (MAX_NODES - 1)

RESOURCES = ['mutex_A', 'mutex_B', 'mutex_C', 'semaphore_1', 'semaphore_2', 'rwlock_X']
EVENT_TYPES = ['WAIT', 'HOLD', 'RELEASE']

os.makedirs(os.path.join(ROOT_DIR, 'status'), exist_ok=True)
os.makedirs(os.path.join(ROOT_DIR, 'logs'),   exist_ok=True)

def scenario_cycle():
    """
    Simulate a realistic scenario cycle:
      0–30s   : safe normal workload
      30–60s  : rising contention
      60–80s  : deadlock escalation
      80–90s  : resolution
    """
    t = 0
    tick = 0
    log_f = open(LOG_PATH, 'w')

    def write_log(event_type, pid, resource):
        ts = datetime.now().strftime('[%H:%M:%S]')
        log_f.write(f'{ts} {event_type} pid={pid} resource={resource}\n')
        log_f.flush()

    try:
        while True:
            phase = t % 90

            if phase < 30:
                # Safe
                blocked   = random.randint(0, 1)
                edge_count= random.randint(2, 6)
                wg        = round(random.uniform(0.0, 0.15), 4)
                risk      = random.uniform(0.02, 0.15)
                status    = 'running'
                evt       = random.choice(['WAIT', 'HOLD', 'RELEASE'])
            elif phase < 60:
                # Contention
                prog      = (phase - 30) / 30
                blocked   = int(1 + prog * 3)
                edge_count= int(6 + prog * 10)
                wg        = round(0.2 + prog * 0.6, 4)
                risk      = 0.2 + prog * 0.5
                status    = 'running'
                evt       = random.choice(['WAIT', 'HOLD', 'WAIT', 'HOLD'])
            elif phase < 80:
                # Deadlock
                prog      = (phase - 60) / 20
                blocked   = int(3 + prog * 3)
                edge_count= int(16 + prog * 8)
                wg        = round(0.8 + prog * 0.8, 4)
                risk      = 0.75 + prog * 0.24
                status    = 'deadlock'
                evt       = random.choice(['WAIT', 'HOLD', 'DEADLOCK'])
            else:
                # Resolution
                blocked   = 0
                edge_count= random.randint(1, 3)
                wg        = round(random.uniform(0.0, 0.05), 4)
                risk      = random.uniform(0.01, 0.1)
                status    = 'running'
                evt       = 'RELEASE'

            density = round(edge_count / MAX_EDGES, 6)
            pid     = random.randint(1000, 9999)
            resource= random.choice(RESOURCES)

            # Write status files
            with open(FEATURES_PATH, 'w') as f:
                json.dump({
                    'blocked_count':    blocked,
                    'wait_time_growth': wg,
                    'edge_count':       edge_count,
                    'graph_density':    density,
                }, f)

            with open(HEARTBEAT_PATH, 'w') as f:
                json.dump({'tick': tick, 'status': status}, f)

            action = 'triggered_resolution' if status == 'deadlock' and risk >= 0.75 else 'none'
            with open(RISK_PATH, 'w') as f:
                json.dump({
                    'timestamp': datetime.now().isoformat(),
                    'risk_score': round(min(risk, 1.0), 4),
                    'action': action,
                }, f)

            write_log(evt, pid, resource)

            tick += 1
            t    += 0.5
            time.sleep(0.5)
    finally:
        log_f.close()

if __name__ == '__main__':
    print("AI-DeadlockGuard  —  Demo Mock Data Generator")
    print("Simulating realistic OS telemetry every 500ms...")
    print("Press Ctrl+C to stop.\n")
    scenario_cycle()
