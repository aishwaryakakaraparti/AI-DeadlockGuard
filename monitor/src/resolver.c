/*
 * monitor/src/resolver.c — Phase 7
 *
 * Signal-based deadlock resolution.
 *
 * resolve_auto():
 *   Finds the first PID-typed node in the cycle path, sends SIGKILL,
 *   clears that node from the graph, and logs the action.
 *
 * resolve_manual():
 *   Kills every process that currently has at least one outgoing WAIT edge
 *   (i.e. every thread waiting for a resource). Then wipes the graph.
 *
 * check_manual_trigger():
 *   Polls RESOLVE_TRIGGER_PATH. If it exists, removes it and returns 1.
 */

#include "resolver.h"
#include "../../ipc/event_protocol.h"
#include "ipc_reader.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

/* ── Logging helper ─────────────────────────────────────────────────────── */

#define LOG_PATH "logs/monitor_events.log"

static void log_resolution(const char *msg, pid_t victim) {
    /* Print to stderr (visible in terminal) */
    fprintf(stderr, "[resolver] %s (pid=%d)\n", msg, (int)victim);

    /* Append to log file */
    FILE *f = fopen(LOG_PATH, "a");
    if (f) {
        time_t now = time(NULL);
        char ts[32];
        strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", localtime(&now));
        fprintf(f, "%s | %s (pid=%d)\n", ts, msg, (int)victim);
        fclose(f);
    }
}

/* ── Automatic resolution ────────────────────────────────────────────────── */

void resolve_auto(graph_t *g, const cycle_result_t *result) {
    if (result->length == 0) return;

    pid_t victim = -1;
    int   victim_node = -1;

    /* Walk the cycle path and pick the first thread node */
    for (int i = 0; i < result->length; i++) {
        int node = result->path[i];
        if (IS_THREAD_NODE(node)) {
            /* Map thread-node index back to a real PID */
            victim = ipc_reader_get_pid(node);   /* node IS the thread index */
            victim_node = node;
            break;
        }
    }

    if (victim <= 0) {
        fprintf(stderr, "[resolver] auto: could not identify victim PID\n");
        return;
    }

    log_resolution("AUTO-RESOLVE: sending SIGKILL to victim", victim);

    if (kill(victim, SIGKILL) < 0) {
        if (errno == ESRCH) {
            fprintf(stderr, "[resolver] victim pid=%d already dead\n", (int)victim);
        } else {
            perror("[resolver] kill");
        }
    }

    /* Remove all graph edges for the victim so the next cycle check is clean */
    graph_clear_node(g, victim_node);
}

/* ── Manual resolution ───────────────────────────────────────────────────── */

void resolve_manual(graph_t *g) {
    fprintf(stderr, "[resolver] MANUAL-RESOLVE: killing all blocked processes\n");

    /*
     * Find every thread node (0..MAX_THREADS-1) that has at least one
     * outgoing edge (i.e. it is waiting for some resource).
     */
    for (int t = 0; t < MAX_THREADS; t++) {
        if (!g->active[t]) continue;

        int waiting = 0;
        for (int r = MAX_THREADS; r < MAX_NODES; r++) {
            if (g->adj[t][r]) { waiting = 1; break; }
        }
        if (!waiting) continue;

        pid_t victim = ipc_reader_get_pid(t);
        if (victim <= 0) continue;

        log_resolution("MANUAL-RESOLVE: sending SIGKILL to blocked process", victim);

        if (kill(victim, SIGKILL) < 0 && errno != ESRCH) {
            perror("[resolver] kill (manual)");
        }
    }

    /* Wipe the graph entirely after manual resolution */
    graph_init(g);
    fprintf(stderr, "[resolver] graph cleared after manual resolution\n");
}

/* ── Trigger file polling ────────────────────────────────────────────────── */

int check_manual_trigger(void) {
    struct stat st;
    if (stat(RESOLVE_TRIGGER_PATH, &st) == 0) {
        /* File exists — consume it */
        unlink(RESOLVE_TRIGGER_PATH);
        fprintf(stderr, "[resolver] manual trigger file detected\n");
        return 1;
    }
    return 0;
}
