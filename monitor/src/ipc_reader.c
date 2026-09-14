/*
 * monitor/src/ipc_reader.c — Phase 6
 *
 * Translates raw ipc_event_t messages from the FIFO into graph operations.
 * This is the only file that knows both the IPC protocol and the graph API.
 *
 * Node-ID assignment strategy:
 *   PIDs are mapped to thread-node indices (0..MAX_THREADS-1) on first seen.
 *   Resource names are mapped to resource-node indices (0..MAX_RESOURCES-1)
 *   on first seen. Both tables live here as static arrays.
 */

#include "ipc_reader.h"
#include "../../ipc/event_protocol.h"
#include "../../ipc/fifo_channel.h"
#include <stdio.h>
#include <string.h>

/* ── PID → thread-node table ─────────────────────────────────────────────── */
static pid_t  pid_table[MAX_THREADS];
static int    pid_count = 0;

static int pid_to_node(graph_t *g, pid_t pid) {
    /* Look up existing mapping */
    for (int i = 0; i < pid_count; i++) {
        if (pid_table[i] == pid) return THREAD_NODE(i);
    }
    /* New PID */
    if (pid_count >= MAX_THREADS) {
        fprintf(stderr, "ipc_reader: PID table full\n");
        return -1;
    }
    int idx = pid_count++;
    pid_table[idx] = pid;
    char name[32];
    snprintf(name, sizeof(name), "pid_%d", (int)pid);
    graph_set_name(g, THREAD_NODE(idx), name);
    return THREAD_NODE(idx);
}

/* ── Resource-name → resource-node table ─────────────────────────────────── */
static char res_table[MAX_RESOURCES][MAX_RESOURCE_NAME];
static int  res_count = 0;

static int resource_to_node(graph_t *g, const char *name) {
    for (int i = 0; i < res_count; i++) {
        if (strncmp(res_table[i], name, MAX_RESOURCE_NAME) == 0)
            return RESOURCE_NODE(i);
    }
    if (res_count >= MAX_RESOURCES) {
        fprintf(stderr, "ipc_reader: resource table full\n");
        return -1;
    }
    int idx = res_count++;
    strncpy(res_table[idx], name, MAX_RESOURCE_NAME - 1);
    graph_set_name(g, RESOURCE_NODE(idx), name);
    return RESOURCE_NODE(idx);
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void ipc_reader_reset(void) {
    pid_count = 0;
    res_count = 0;
    memset(pid_table,  0, sizeof(pid_table));
    memset(res_table,  0, sizeof(res_table));
}

int ipc_reader_apply(graph_t *g, const ipc_event_t *ev) {
    if (ev->type == EV_DONE) {
        /* Worker exiting — clean up its node from the graph */
        for (int i = 0; i < pid_count; i++) {
            if (pid_table[i] == ev->pid) {
                graph_clear_node(g, THREAD_NODE(i));
                pid_table[i] = 0;   /* mark slot free (simple approach) */
                break;
            }
        }
        return 0;
    }

    int t_node = pid_to_node(g, ev->pid);
    int r_node = resource_to_node(g, ev->resource);
    if (t_node < 0 || r_node < 0) return -1;

    switch (ev->type) {
        case EV_WAIT:
            /* Thread → resource: thread is blocking on this resource */
            graph_add_edge(g, t_node, r_node);
            fprintf(stderr, "[monitor] WAIT  pid=%-6d resource=%s\n",
                    (int)ev->pid, ev->resource);
            break;

        case EV_HOLD:
            /* WAIT edge done; now resource → thread: resource held by thread */
            graph_remove_edge(g, t_node, r_node);
            graph_add_edge(g, r_node, t_node);
            fprintf(stderr, "[monitor] HOLD  pid=%-6d resource=%s\n",
                    (int)ev->pid, ev->resource);
            break;

        case EV_RELEASE:
            /* Resource released: remove resource → thread edge */
            graph_remove_edge(g, r_node, t_node);
            fprintf(stderr, "[monitor] RELEASE pid=%-6d resource=%s\n",
                    (int)ev->pid, ev->resource);
            break;

        default:
            fprintf(stderr, "[monitor] unknown event type %d\n", ev->type);
            return -1;
    }
    return 0;
}

pid_t ipc_reader_get_pid(int thread_node_idx) {
    if (thread_node_idx < 0 || thread_node_idx >= pid_count) return -1;
    return pid_table[thread_node_idx];
}
