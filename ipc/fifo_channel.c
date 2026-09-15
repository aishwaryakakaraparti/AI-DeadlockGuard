/*
 * ipc/fifo_channel.c — Phase 6
 *
 * Implementation of the FIFO channel. Workers write fixed-size ipc_event_t
 * structs; the monitor reads them one at a time with a simple blocking read.
 *
 * Why fixed-size structs over the FIFO?
 *   - No framing/parsing needed — one read() always gives exactly one event.
 *   - Writes ≤ PIPE_BUF (4096 bytes) are atomic on Linux, so concurrent
 *     worker writes never interleave partial messages.
 */

#include "fifo_channel.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* ── Writer side ─────────────────────────────────────────────────────────── */

int fifo_open_write(void) {
    /* O_WRONLY blocks until a reader opens the other end — that's fine;
     * the monitor must be started before the workers. */
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd < 0) {
        perror("fifo_open_write: open");
    }
    return fd;
}

int fifo_send_event(int fd, event_type_t type, pid_t pid, const char *resource) {
    ipc_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.pid  = pid;
    strncpy(ev.resource, resource, MAX_RESOURCE_NAME - 1);

    ssize_t n = write(fd, &ev, sizeof(ev));
    if (n != (ssize_t)sizeof(ev)) {
        perror("fifo_send_event: write");
        return -1;
    }
    return 0;
}

void fifo_close_write(int fd) {
    close(fd);
}

/* ── Reader side ─────────────────────────────────────────────────────────── */

int fifo_open_read(void) {
    /* Create FIFO if it doesn't exist */
    if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
        perror("fifo_open_read: mkfifo");
        return -1;
    }

    /*
     * Open with O_RDWR instead of O_RDONLY so the fd stays open even when
     * all current writers have closed (no spurious EOF until we explicitly
     * want it). This is the standard trick to keep a FIFO readable across
     * multiple writer lifetimes.
     */
    int fd = open(FIFO_PATH, O_RDWR);
    if (fd < 0) {
        perror("fifo_open_read: open");
        return -1;
    }
    return fd;
}

int fifo_recv_event(int fd, ipc_event_t *ev) {
    ssize_t n = read(fd, ev, sizeof(*ev));
    if (n == 0) {
        return 0;  /* EOF */
    }
    if (n < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;  /* no data or interrupted */
        }
        perror("fifo_recv_event: read");
        return -1;
    }
    if (n != (ssize_t)sizeof(*ev)) {
        fprintf(stderr, "fifo_recv_event: short read (%zd bytes)\n", n);
        return -1;
    }
    return 1;
}

void fifo_destroy(void) {
    unlink(FIFO_PATH);
}
