/*
 * monitor/include/ipc_reader.h — Phase 6
 */

#pragma once

#include "graph.h"
#include "../../ipc/event_protocol.h"

/*
 * Reset the PID and resource name tables (call before a new scenario run).
 */
void ipc_reader_reset(void);

/*
 * Apply one IPC event to the graph.
 * Returns 0 on success, -1 on error (table full, unknown event, etc.)
 */
int ipc_reader_apply(graph_t *g, const ipc_event_t *ev);

/*
 * Look up the real PID for a thread-node index (used by resolver).
 * Returns -1 if index is out of range.
 */
pid_t ipc_reader_get_pid(int thread_node_idx);
