#include "common.h"

void log_event(event_type_t type, pid_t tid, const char *resource) {
    switch (type) {
        case EVENT_WAIT:
            fprintf(stderr, "WAIT tid=%d resource=%s\n", tid, resource);
            break;
        case EVENT_HOLD:
            fprintf(stderr, "HOLD tid=%d resource=%s\n", tid, resource);
            break;
        case EVENT_RELEASE:
            fprintf(stderr, "RELEASE tid=%d resource=%s\n", tid, resource);
            break;
    }
}

void tracked_lock(pthread_mutex_t *mutex, const char *resource_name) {
    log_event(EVENT_WAIT, get_tid(), resource_name);
    pthread_mutex_lock(mutex);
    log_event(EVENT_HOLD, get_tid(), resource_name);
}

void tracked_unlock(pthread_mutex_t *mutex, const char *resource_name) {
    log_event(EVENT_RELEASE, get_tid(), resource_name);
    pthread_mutex_unlock(mutex);
}
