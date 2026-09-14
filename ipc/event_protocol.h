/*
 * ipc/event_protocol.h — Phase 6
 *
 * Wire format for events sent from worker processes to the monitor daemon
 * over a named pipe (FIFO). All messages are fixed-size structs for
 * simple, framing-free reads on the monitor side.
 *
 * Event flow:
 *   Worker calls tracked_lock()  → sends EVENT_WAIT then EVENT_HOLD
 *   Worker calls tracked_unlock() → sends EVENT_RELEASE
 *
 * The monitor reads these and maintains the wait-for graph.
 */

#pragma once

#include <sys/types.h>  /* pid_t */
#include <stdint.h>

/* Path for the named pipe (FIFO) */
#define FIFO_PATH "/tmp/deadlock_guard.fifo"

/* Path for manual-resolve trigger file (Phase 7) */
#define RESOLVE_TRIGGER_PATH "/tmp/deadlock_guard.resolve"

/* Maximum resource name length */
#define MAX_RESOURCE_NAME 32

/* Event types — must match log_event / tracked_lock semantics */
typedef enum {
    EV_WAIT    = 1,   /* process is blocking on a mutex */
    EV_HOLD    = 2,   /* process has acquired a mutex   */
    EV_RELEASE = 3,   /* process has released a mutex   */
    EV_DONE    = 99   /* sentinel: worker process exiting */
} event_type_t;

/*
 * ipc_event_t — one fixed-size message over the FIFO.
 * Kept to 64 bytes so a single write() is always atomic on Linux
 * (pipe atomicity guaranteed for writes ≤ PIPE_BUF = 4096 bytes).
 */
typedef struct {
    event_type_t type;                  /* EV_WAIT / EV_HOLD / EV_RELEASE / EV_DONE */
    pid_t        pid;                   /* sending process ID                         */
    char         resource[MAX_RESOURCE_NAME]; /* lock name, e.g. "fork_0"             */
    uint8_t      _pad[64 - sizeof(event_type_t) - sizeof(pid_t) - MAX_RESOURCE_NAME];
} ipc_event_t;
