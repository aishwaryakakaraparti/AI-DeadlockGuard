#include "common.h"

void log_event(log_event_type_t type, pid_t tid, const char *resource) {
    const char *type_str = "";
    switch (type) {
        case EVENT_WAIT:    type_str = "WAIT"; break;
        case EVENT_HOLD:    type_str = "HOLD"; break;
        case EVENT_RELEASE: type_str = "RELEASE"; break;
    }
    fprintf(stderr, "%-7s pid=%d resource=%s\n", type_str, (int)tid, resource);
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
