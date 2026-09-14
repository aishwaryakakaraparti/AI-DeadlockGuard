/*
 * monitor/include/resolver.h — Phase 7
 *
 * Signal-based deadlock resolution.
 *
 * Two resolution paths:
 *   1. Automatic: called by the monitor when detect_cycle() returns true.
 *   2. Manual:    triggered externally (GUI button / shell command) by
 *                 touching RESOLVE_TRIGGER_PATH. The monitor's event loop
 *                 polls for this file and calls resolve_manual() when found.
 */

#pragma once

#include "graph.h"
#include "cycle_detector.h"
#include <sys/types.h>

/*
 * resolve_auto() — kill one process from the detected cycle to break it.
 *
 * Strategy: victim = first PID-typed node in the cycle path.
 * Sends SIGKILL; logs the action to stderr and to the events log file.
 *
 * After killing, clears that node's edges from the graph so subsequent
 * cycle checks are clean.
 */
void resolve_auto(graph_t *g, const cycle_result_t *result);

/*
 * resolve_manual() — forcibly kill all processes that currently have at
 * least one outgoing WAIT edge (i.e. every blocked process), then clear
 * the graph entirely.
 *
 * Called when the operator hits "Resolve Deadlock" in the GUI or touches
 * the trigger file.
 */
void resolve_manual(graph_t *g);

/*
 * check_manual_trigger() — returns 1 if RESOLVE_TRIGGER_PATH exists,
 * 0 otherwise. Removes the file after detecting it so it fires once.
 */
int check_manual_trigger(void);
