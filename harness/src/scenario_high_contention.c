/*
 * scenario_high_contention.c — Phase 5
 *
 * Multiple worker processes competing heavily for a pool of shared resources,
 * but with consistent lock ordering — NO deadlock should ever occur.
 *
 * Purpose: validates that the detection system does NOT false-alarm
 * under legitimate high contention.
 *
 * Design:
 *   - 4 worker processes, 3 shared mutexes (R0, R1, R2)
 *   - Each worker randomly selects 2 resources to lock
 *   - Always locks the lower-numbered resource first (resource ordering)
 *   - Repeats for several iterations with randomized delays
 *
 * Demonstrates: fork(), mmap, process-shared mutexes, high contention
 *               without deadlock (resource ordering guarantee).
 */

#include "common.h"
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

#define N_WORKERS    4
#define N_RESOURCES  3
#define N_ITERATIONS 5

typedef struct {
    pthread_mutex_t resources[N_RESOURCES];
} shared_state_t;

static shared_state_t *shared;

static const char *resource_names[] = { "res_0", "res_1", "res_2" };

static void worker(int id) {
    /* Seed RNG differently per process */
    srand((unsigned)(time(NULL) ^ (getpid() << 8)));

    for (int iter = 0; iter < N_ITERATIONS; iter++) {
        /* Pick two distinct resources randomly */
        int a = rand() % N_RESOURCES;
        int b;
        do {
            b = rand() % N_RESOURCES;
        } while (b == a);

        /* Resource ordering: always lock lower-numbered first */
        int first  = (a < b) ? a : b;
        int second = (a < b) ? b : a;

        fprintf(stderr, "[Worker %d (pid=%d)] iter %d: locking %s then %s\n",
                id, getpid(), iter, resource_names[first], resource_names[second]);

        tracked_lock(&shared->resources[first], resource_names[first]);

        /* Simulate some work while holding first lock */
        usleep(10000 + (rand() % 50000));

        tracked_lock(&shared->resources[second], resource_names[second]);

        fprintf(stderr, "[Worker %d (pid=%d)] iter %d: holding both %s and %s — working\n",
                id, getpid(), iter, resource_names[first], resource_names[second]);

        /* Simulate critical section work */
        usleep(20000 + (rand() % 30000));

        /* Release in reverse order */
        tracked_unlock(&shared->resources[second], resource_names[second]);
        tracked_unlock(&shared->resources[first],  resource_names[first]);

        fprintf(stderr, "[Worker %d (pid=%d)] iter %d: released both locks\n",
                id, getpid(), iter);

        /* Brief pause between iterations */
        usleep(5000 + (rand() % 20000));
    }
}

int main(void) {
    /* Allocate shared memory for the resources (mutexes) */
    shared = mmap(NULL, sizeof(shared_state_t),
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* Initialize mutexes with PTHREAD_PROCESS_SHARED */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    for (int i = 0; i < N_RESOURCES; i++) {
        pthread_mutex_init(&shared->resources[i], &attr);
    }
    pthread_mutexattr_destroy(&attr);

    fprintf(stderr, "=== High Contention (NO deadlock — resource ordering) ===\n");
    fprintf(stderr, "Launching %d worker processes with %d resources, %d iterations each\n",
            N_WORKERS, N_RESOURCES, N_ITERATIONS);

    /* Fork worker processes */
    pid_t pids[N_WORKERS];
    for (int i = 0; i < N_WORKERS; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return 1;
        }
        if (pids[i] == 0) {
            worker(i);
            _exit(0);
        }
    }

    /* Parent waits for all children */
    for (int i = 0; i < N_WORKERS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    /* Cleanup */
    for (int i = 0; i < N_RESOURCES; i++) {
        pthread_mutex_destroy(&shared->resources[i]);
    }
    munmap(shared, sizeof(shared_state_t));

    fprintf(stderr, "All workers finished successfully — no deadlock occurred.\n");
    return 0;
}
