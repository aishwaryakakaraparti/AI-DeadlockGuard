/*
 * scenario_dining_philosophers_fixed.c — Phase 5
 *
 * Fixed dining philosophers with separate processes (fork).
 * 5 philosophers, 5 forks (shared mutexes via mmap).
 * Uses resource ordering to prevent deadlock:
 *   - Each philosopher always picks up the LOWER-numbered fork first.
 *   - This breaks the circular wait condition.
 *
 * Demonstrates: fork(), mmap(MAP_SHARED|MAP_ANONYMOUS), process-shared mutexes,
 *               deadlock prevention via resource ordering.
 */

#include "common.h"
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

#define N_PHILOSOPHERS 5

typedef struct {
    pthread_mutex_t forks[N_PHILOSOPHERS];
} shared_state_t;

static shared_state_t *shared;

static void philosopher(int id) {
    int left  = id;
    int right = (id + 1) % N_PHILOSOPHERS;

    /* Resource ordering: always lock the lower-numbered fork first */
    int first  = (left < right) ? left  : right;
    int second = (left < right) ? right : left;

    char first_name[32], second_name[32];
    snprintf(first_name,  sizeof(first_name),  "fork_%d", first);
    snprintf(second_name, sizeof(second_name), "fork_%d", second);

    /* Think briefly */
    usleep(10000 * id);

    fprintf(stderr, "[Philosopher %d (pid=%d)] Thinking...\n", id, getpid());

    /* Pick up lower-numbered fork first → no circular wait possible */
    tracked_lock(&shared->forks[first], first_name);
    fprintf(stderr, "[Philosopher %d (pid=%d)] Picked up fork %d (first)\n",
            id, getpid(), first);

    usleep(50000);  /* simulate delay between acquiring forks */

    tracked_lock(&shared->forks[second], second_name);
    fprintf(stderr, "[Philosopher %d (pid=%d)] Picked up fork %d (second) — EATING\n",
            id, getpid(), second);

    /* Eat */
    usleep(100000);

    /* Put down forks in reverse order */
    tracked_unlock(&shared->forks[second], second_name);
    tracked_unlock(&shared->forks[first],  first_name);

    fprintf(stderr, "[Philosopher %d (pid=%d)] Done eating, forks released\n",
            id, getpid());
}

int main(void) {
    /* Allocate shared memory for the forks (mutexes) */
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

    for (int i = 0; i < N_PHILOSOPHERS; i++) {
        pthread_mutex_init(&shared->forks[i], &attr);
    }
    pthread_mutexattr_destroy(&attr);

    fprintf(stderr, "=== Dining Philosophers (FIXED version — resource ordering) ===\n");
    fprintf(stderr, "Launching %d philosopher processes...\n", N_PHILOSOPHERS);

    /* Fork a child process for each philosopher */
    pid_t pids[N_PHILOSOPHERS];
    for (int i = 0; i < N_PHILOSOPHERS; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return 1;
        }
        if (pids[i] == 0) {
            philosopher(i);
            _exit(0);
        }
    }

    /* Parent waits for all children */
    for (int i = 0; i < N_PHILOSOPHERS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    /* Cleanup */
    for (int i = 0; i < N_PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&shared->forks[i]);
    }
    munmap(shared, sizeof(shared_state_t));

    fprintf(stderr, "All philosophers finished successfully — no deadlock occurred.\n");
    return 0;
}
