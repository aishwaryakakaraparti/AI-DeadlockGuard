#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "graph.h"
#include "cycle_detector.h"

graph_t g;
pthread_mutex_t graph_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_A;
pthread_mutex_t mutex_B;

#define THREAD1_NODE  THREAD_NODE(0)
#define THREAD2_NODE  THREAD_NODE(1)  
#define MUTEX_A_NODE  RESOURCE_NODE(0)
#define MUTEX_B_NODE  RESOURCE_NODE(1)

void report_event(int event_type, int thread_node, int resource_node) {
    cycle_result_t result;
    
    pthread_mutex_lock(&graph_mutex);
    
    if (event_type == 0) { // WAIT
        graph_add_edge(&g, thread_node, resource_node);
        if (detect_cycle(&g, &result)) {
            printf("🔴 ");
            print_cycle(&g, &result);
        }
    } else if (event_type == 1) { // HOLD
        graph_remove_edge(&g, thread_node, resource_node);
        graph_add_edge(&g, resource_node, thread_node);
    } else if (event_type == 2) { // RELEASE
        graph_remove_edge(&g, resource_node, thread_node);
    }
    
    pthread_mutex_unlock(&graph_mutex);
}

void* thread1_func(void* arg) {
    (void)arg;
    
    report_event(0, THREAD1_NODE, MUTEX_A_NODE);
    pthread_mutex_lock(&mutex_A);
    report_event(1, THREAD1_NODE, MUTEX_A_NODE);
    
    printf("Thread 1: Acquired Mutex A\n");
    sleep(1);
    
    report_event(0, THREAD1_NODE, MUTEX_B_NODE);
    pthread_mutex_lock(&mutex_B);
    report_event(1, THREAD1_NODE, MUTEX_B_NODE);
    
    printf("Thread 1: Acquired Mutex B\n");
    
    report_event(2, THREAD1_NODE, MUTEX_B_NODE);
    pthread_mutex_unlock(&mutex_B);
    report_event(2, THREAD1_NODE, MUTEX_A_NODE);
    pthread_mutex_unlock(&mutex_A);
    
    return NULL;
}

void* thread2_func(void* arg) {
    (void)arg;
    
    report_event(0, THREAD2_NODE, MUTEX_B_NODE);
    pthread_mutex_lock(&mutex_B);
    report_event(1, THREAD2_NODE, MUTEX_B_NODE);
    
    printf("Thread 2: Acquired Mutex B\n");
    sleep(1);
    
    report_event(0, THREAD2_NODE, MUTEX_A_NODE);
    pthread_mutex_lock(&mutex_A);
    report_event(1, THREAD2_NODE, MUTEX_A_NODE);
    
    printf("Thread 2: Acquired Mutex A\n");
    
    report_event(2, THREAD2_NODE, MUTEX_A_NODE);
    pthread_mutex_unlock(&mutex_A);
    report_event(2, THREAD2_NODE, MUTEX_B_NODE);
    pthread_mutex_unlock(&mutex_B);
    
    return NULL;
}

int main() {
    graph_init(&g);
    graph_set_name(&g, THREAD1_NODE, "Thread_1");
    graph_set_name(&g, THREAD2_NODE, "Thread_2");
    graph_set_name(&g, MUTEX_A_NODE, "mutex_A");
    graph_set_name(&g, MUTEX_B_NODE, "mutex_B");
    
    pthread_mutex_init(&mutex_A, NULL);
    pthread_mutex_init(&mutex_B, NULL);
    
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("Done\n");
    return 0;
}
