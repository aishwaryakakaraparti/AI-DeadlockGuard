#include "common.h"

pthread_mutex_t mutex_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_B = PTHREAD_MUTEX_INITIALIZER;

void *thread1_func(void *arg) {
    (void)arg;
    tracked_lock(&mutex_A, "mutex_A");
    printf("Thread 1: holding mutex_A, sleeping before acquiring mutex_B...\n");
    usleep(100000); // 100ms
    tracked_lock(&mutex_B, "mutex_B");
    printf("Thread 1: acquired both locks (this should never print)\n");
    tracked_unlock(&mutex_B, "mutex_B");
    tracked_unlock(&mutex_A, "mutex_A");
    return NULL;
}

void *thread2_func(void *arg) {
    (void)arg;
    tracked_lock(&mutex_B, "mutex_B");
    printf("Thread 2: holding mutex_B, sleeping before acquiring mutex_A...\n");
    usleep(100000); // 100ms
    tracked_lock(&mutex_A, "mutex_A");
    printf("Thread 2: acquired both locks (this should never print)\n");
    tracked_unlock(&mutex_A, "mutex_A");
    tracked_unlock(&mutex_B, "mutex_B");
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&mutex_A);
    pthread_mutex_destroy(&mutex_B);

    printf("Program completed (no deadlock occurred)\n");
    return 0;
}
