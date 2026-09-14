/*
 * scenario_dining_philosophers.c — Phase 5
 *
 * Classic dining philosophers with separate processes (fork).
 * 5 philosophers, 5 forks (shared mutexes via mmap).
 * Each philosopher picks up LEFT fork first, then RIGHT → deadlock.
 *
 * Shared memory layout:
 *   - 5 pthread_mutex_t (chopsticks), initialized with PTHREAD_PROCESS_SHARED
 *
 * Demonstrates: fork(), mmap(MAP_SHARED|MAP_ANONYMOUS), process-shared mutexes
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

    char left_name[32], right_name[32];
    snprintf(left_name,  sizeof(left_name),  "fork_%d", left);
    snprintf(right_name, sizeof(right_name), "fork_%d", right);

    /* Think briefly */
    usleep(10000 * id);  /* stagger start to make interleaving visible */

    fprintf(stderr, "[Philosopher %d (pid=%d)] Thinking...\n", id, getpid());

    /* Pick up LEFT fork first, then RIGHT → circular wait → deadlock */
    tracked_lock(&shared->forks[left], left_name);
    fprintf(stderr, "[Philosopher %d (pid=%d)] Picked up left fork %d\n",
            id, getpid(), left);

    usleep(50000);  /* hold left fork, wait before grabbing right — ensures all
                       philosophers hold their left fork simultaneously */

    tracked_lock(&shared->forks[right], right_name);
    fprintf(stderr, "[Philosopher %d (pid=%d)] Picked up right fork %d — EATING\n",
            id, getpid(), right);

    /* Eat (this should never print if deadlocked) */
    usleep(100000);

    /* Put down forks */
    tracked_unlock(&shared->forks[right], right_name);
    tracked_unlock(&shared->forks[left],  left_name);

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

    fprintf(stderr, "=== Dining Philosophers (DEADLOCK version) ===\n");
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
            /* Child process */
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

    fprintf(stderr, "All philosophers finished (no deadlock occurred).\n");
    return 0;
}
