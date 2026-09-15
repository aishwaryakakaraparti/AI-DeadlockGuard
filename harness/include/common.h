#pragma once

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

static inline pid_t get_tid(void) { return (pid_t)syscall(SYS_gettid); }

typedef enum {
    EVENT_WAIT,
    EVENT_HOLD,
    EVENT_RELEASE
} log_event_type_t;

void log_event(log_event_type_t type, pid_t tid, const char *resource);
void tracked_lock(pthread_mutex_t *mutex, const char *resource_name);
void tracked_unlock(pthread_mutex_t *mutex, const char *resource_name);
