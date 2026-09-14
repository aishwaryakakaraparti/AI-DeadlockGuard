/*
 * ipc/fifo_channel.h — Phase 6
 *
 * Writer-side (worker) and reader-side (monitor) API for the FIFO channel.
 */

#pragma once

#include "event_protocol.h"

/* ── Writer side (used by worker processes) ─────────────────────────────── */

/*
 * Open the FIFO for writing. Blocks until the monitor opens it for reading.
 * Returns fd on success, -1 on error.
 */
int fifo_open_write(void);

/*
 * Send a single event over the FIFO.
 * Returns 0 on success, -1 on error (e.g. monitor died).
 */
int fifo_send_event(int fd, event_type_t type, pid_t pid, const char *resource);

/*
 * Close the write end.
 */
void fifo_close_write(int fd);

/* ── Reader side (used by monitor daemon) ───────────────────────────────── */

/*
 * Create the FIFO if it doesn't exist, then open it for reading (non-blocking
 * so the monitor doesn't block waiting for the first writer). Switches to
 * blocking mode after open.
 * Returns fd on success, -1 on error.
 */
int fifo_open_read(void);

/*
 * Read one event from the FIFO into *ev.
 * Returns 1 on success, 0 on EOF (all writers closed), -1 on error.
 */
int fifo_recv_event(int fd, ipc_event_t *ev);

/*
 * Remove the FIFO from the filesystem.
 */
void fifo_destroy(void);
