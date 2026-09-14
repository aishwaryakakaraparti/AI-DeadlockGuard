/*
 * harness/src/scenario_circular_wait_ipc.c — Phase 6
 *
 * Two-process circular wait scenario that sends WAIT/HOLD/RELEASE events
 * to the monitor daemon over the FIFO.
 *
 * Run this AFTER starting the monitor:
 *   ./monitor/build/monitor --auto-resolve &
 *   ./harness/build/circular_wait_ipc
 *
 * Each worker is a separate forked process (real process isolation).
 * They share two mutexes via mmap (same technique as Phase 5).
 *
 * The monitor receives the events, builds the wait-for graph, detects the
 * cycle, and (with --auto-resolve) SIGKILLs one worker to break the deadlock.
 */

#include "common.h"
#include "ipc_worker.h"
#include <sys/mman.h>
#include <sys/wait.h>

typedef struct {
    pthread_mutex_t mutex_A;
    pthread_mutex_t mutex_B;
} shared_locks_t;

static shared_locks_t *locks;

/* ── Tracked lock wrappers that also send IPC events ────────────────────── */

static void ipc_lock(pthread_mutex_t *m, const char *name) {
    log_event(EVENT_WAIT, getpid(), name);
    ipc_worker_send(EV_WAIT, name);
    pthread_mutex_lock(m);
    log_event(EVENT_HOLD, getpid(), name);
    ipc_worker_send(EV_HOLD, name);
}

static void ipc_unlock(pthread_mutex_t *m, const char *name) {
    log_event(EVENT_RELEASE, getpid(), name);
    ipc_worker_send(EV_RELEASE, name);
    pthread_mutex_unlock(m);
}

/* ── Worker A: locks mutex_A then mutex_B ───────────────────────────────── */
static void worker_a(void) {
    ipc_worker_init();

    fprintf(stderr, "[Worker A pid=%d] Attempting mutex_A then mutex_B\n", getpid());

    ipc_lock(&locks->mutex_A, "mutex_A");
    fprintf(stderr, "[Worker A pid=%d] Holding mutex_A, sleeping...\n", getpid());
    usleep(200000);   /* 200ms — ensures Worker B grabs mutex_B first */

    ipc_lock(&locks->mutex_B, "mutex_B");   /* blocks — Worker B holds it */
    fprintf(stderr, "[Worker A pid=%d] Holding both (should not print)\n", getpid());

    ipc_unlock(&locks->mutex_B, "mutex_B");
    ipc_unlock(&locks->mutex_A, "mutex_A");

    ipc_worker_fini();
    _exit(0);
}

/* ── Worker B: locks mutex_B then mutex_A ───────────────────────────────── */
static void worker_b(void) {
    ipc_worker_init();

    fprintf(stderr, "[Worker B pid=%d] Attempting mutex_B then mutex_A\n", getpid());

    ipc_lock(&locks->mutex_B, "mutex_B");
    fprintf(stderr, "[Worker B pid=%d] Holding mutex_B, sleeping...\n", getpid());
    usleep(200000);

    ipc_lock(&locks->mutex_A, "mutex_A");   /* blocks — Worker A holds it */
    fprintf(stderr, "[Worker B pid=%d] Holding both (should not print)\n", getpid());

    ipc_unlock(&locks->mutex_A, "mutex_A");
    ipc_unlock(&locks->mutex_B, "mutex_B");

    ipc_worker_fini();
    _exit(0);
}

/* ── Main ────────────────────────────────────────────────────────────────── */
int main(void) {
    /* Shared mutex setup (process-shared) */
    locks = mmap(NULL, sizeof(shared_locks_t),
                 PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (locks == MAP_FAILED) { perror("mmap"); return 1; }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&locks->mutex_A, &attr);
    pthread_mutex_init(&locks->mutex_B, &attr);
    pthread_mutexattr_destroy(&attr);

    fprintf(stderr, "=== Circular Wait IPC (monitor-connected) ===\n");
    fprintf(stderr, "Forking two worker processes...\n");

    pid_t pa = fork();
    if (pa == 0) { worker_a(); }

    usleep(50000);   /* slight stagger so A gets mutex_A first */

    pid_t pb = fork();
    if (pb == 0) { worker_b(); }

    /* Wait for both — monitor will kill one if auto-resolve is on */
    int status;
    waitpid(pa, &status, 0);
    fprintf(stderr, "[parent] Worker A exited (status=%d)\n", WEXITSTATUS(status));
    waitpid(pb, &status, 0);
    fprintf(stderr, "[parent] Worker B exited (status=%d)\n", WEXITSTATUS(status));

    pthread_mutex_destroy(&locks->mutex_A);
    pthread_mutex_destroy(&locks->mutex_B);
    munmap(locks, sizeof(shared_locks_t));
    return 0;
}
