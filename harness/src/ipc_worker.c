/*
 * harness/src/ipc_worker.c — Phase 6
 *
 * Thin wrapper so each worker process can write events to the monitor FIFO.
 * Falls back gracefully if the FIFO doesn't exist (monitor not running).
 */

#include "ipc_worker.h"
#include "../../ipc/fifo_channel.h"
#include <unistd.h>
#include <stdio.h>

static int g_fifo_fd = -1;

int ipc_worker_init(void) {
    g_fifo_fd = fifo_open_write();
    if (g_fifo_fd < 0) {
        fprintf(stderr, "[worker pid=%d] WARNING: could not open FIFO — "
                        "running without monitor\n", (int)getpid());
        return -1;
    }
    return 0;
}

void ipc_worker_send(event_type_t type, const char *resource) {
    if (g_fifo_fd < 0) return;   /* monitor not connected */
    fifo_send_event(g_fifo_fd, type, getpid(), resource);
}

void ipc_worker_fini(void) {
    if (g_fifo_fd < 0) return;
    fifo_send_event(g_fifo_fd, EV_DONE, getpid(), "");
    fifo_close_write(g_fifo_fd);
    g_fifo_fd = -1;
}
