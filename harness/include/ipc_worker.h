/*
 * harness/src/ipc_worker.h — Phase 6
 *
 * Per-process IPC initialisation helpers.
 * Workers call ipc_worker_init() after fork() to open the FIFO write-end,
 * then use ipc_worker_send() instead of (or alongside) log_event().
 * ipc_worker_fini() sends EV_DONE and closes the fd.
 */

#pragma once

#include "../../ipc/event_protocol.h"

/*
 * Open the FIFO for writing. Call once per process (after fork).
 * Returns 0 on success, -1 on failure (monitor not running).
 */
int  ipc_worker_init(void);

/*
 * Send one event to the monitor. pid = getpid().
 */
void ipc_worker_send(event_type_t type, const char *resource);

/*
 * Send EV_DONE and close the FIFO fd.
 */
void ipc_worker_fini(void);
