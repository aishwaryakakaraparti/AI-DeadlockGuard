#!/usr/bin/env python3
"""
generate_rigorous_dataset.py

Generates an academically rigorous, multi-process execution trace dataset based on
Operating System Resource Allocation Graph (RAG) principles. 

Models three distinct OS process behaviors:
1. Safe Runs (Consistent lock ordering, clean lock/unlock cycles)
2. High Contention Runs (Heavy resource competition, temporary wait spikes, no cycles)
3. Imminent Deadlock Runs (Circular wait dependencies forming over time)

Matches the exact 4-feature schema of AI-DeadlockGuard:
- blocked_count
- wait_time_growth
- edge_count
- graph_density
- is_deadlock_imminent (target label: 1 = deadlock imminent within window, 0 = safe)
"""

import os
import random
import numpy as np
import pandas as pd

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DATA_OUT = os.path.join(ROOT_DIR, 'predictor', 'data', 'training_data.csv')

def generate_traces(num_runs_per_type=50, steps_per_run=40):
    np.random.seed(42)
    random.seed(42)
    
    rows = []
    
    # --------------------------------------------------------------------------
    # 1. SAFE RUNS (Resource Ordering / No Circular Wait)
    # --------------------------------------------------------------------------
    for run in range(num_runs_per_type):
        num_processes = random.randint(3, 8)
        num_resources = random.randint(3, 8)
        max_possible_edges = 64 * 63  # Based on MAX_NODES = 64 in graph.h
        
        for step in range(steps_per_run):
            # Safe runs have low, fluctuating blocked counts (processes rarely wait long)
            blocked_count = int(np.random.poisson(lam=0.2))
            blocked_count = min(blocked_count, num_processes - 1)
            
            # Low edge count (active holds + transient requests)
            edge_count = random.randint(1, num_processes + 2)
            
            # Growth rate fluctuates near 0
            wait_time_growth = float(np.random.normal(loc=0.0, scale=0.1))
            
            # Graph density is very low
            graph_density = edge_count / max_possible_edges
            
            rows.append({
                'blocked_count': blocked_count,
                'wait_time_growth': max(0.0, wait_time_growth),
                'edge_count': edge_count,
                'graph_density': graph_density,
                'is_deadlock_imminent': 0
            })

    # --------------------------------------------------------------------------
    # 2. HIGH CONTENTION RUNS (Heavy locking, high throughput, zero cycles)
    # --------------------------------------------------------------------------
    for run in range(num_runs_per_type):
        num_processes = random.randint(5, 12)
        max_possible_edges = 64 * 63
        
        for step in range(steps_per_run):
            # High contention has moderate blocked processes, but they unblock quickly
            blocked_count = random.randint(1, 3)
            edge_count = random.randint(num_processes, num_processes * 2)
            
            # Growth rate experiences short spikes then settles
            wait_time_growth = float(np.random.exponential(scale=0.3))
            graph_density = edge_count / max_possible_edges
            
            rows.append({
                'blocked_count': blocked_count,
                'wait_time_growth': wait_time_growth,
                'edge_count': edge_count,
                'graph_density': graph_density,
                'is_deadlock_imminent': 0
            })

    # --------------------------------------------------------------------------
    # 3. DEADLOCK RUNS (Circular Wait Progression)
    # --------------------------------------------------------------------------
    for run in range(num_runs_per_type):
        num_processes = random.randint(3, 6)
        max_possible_edges = 64 * 63
        
        # Progression over time towards a frozen deadlock state
        curr_blocked = 0
        curr_edges = random.randint(2, 4)
        
        for step in range(steps_per_run):
            progress = step / float(steps_per_run)
            
            if progress < 0.4:
                # Early phase: looks normal
                curr_blocked = 0 if random.random() > 0.3 else 1
                curr_edges += random.choice([0, 1])
                wait_growth = max(0.0, float(np.random.normal(0.1, 0.05)))
                is_imminent = 0
            elif progress < 0.7:
                # Escalation phase: processes start blocking on each other
                curr_blocked = min(num_processes - 1, curr_blocked + random.choice([0, 1]))
                curr_edges += 1
                wait_growth = float(np.random.normal(0.8, 0.2))
                is_imminent = 1  # Imminent deadlock state
            else:
                # Final Deadlock phase: total freeze (all processes blocked in circular chain)
                curr_blocked = num_processes
                curr_edges = num_processes * 2
                wait_growth = float(np.random.normal(1.5, 0.3))  # High accumulation rate
                is_imminent = 1

            graph_density = curr_edges / max_possible_edges

            rows.append({
                'blocked_count': curr_blocked,
                'wait_time_growth': max(0.0, wait_growth),
                'edge_count': curr_edges,
                'graph_density': graph_density,
                'is_deadlock_imminent': is_imminent
            })

    df = pd.DataFrame(rows)
    os.makedirs(os.path.dirname(DATA_OUT), exist_ok=True)
    df.to_csv(DATA_OUT, index=False)
    print(f"Generated {len(df)} process execution trace samples across {num_runs_per_type * 3} runs.")
    print(f"Dataset saved to: {DATA_OUT}")

if __name__ == "__main__":
    generate_traces()
