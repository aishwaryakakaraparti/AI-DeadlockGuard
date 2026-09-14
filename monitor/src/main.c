/*
 * monitor/src/main.c — Phase 6 & 7
 *
 * The monitor daemon. Event loop:
 *
 *   1. Open the FIFO for reading (blocks until first writer connects).
 *   2. For each incoming ipc_event_t:
 *        a. Apply it to the wait-for graph via ipc_reader_apply().
 *        b. Run detect_cycle(). If cycle found → resolve_auto() (Phase 7).
 *        c. Every POLL_INTERVAL_MS, extract features and write features.json.
 *   3. On every iteration also check check_manual_trigger(). If set →
 *      resolve_manual() regardless of whether a cycle exists.
 *   4. When the FIFO returns EOF (all workers done), exit cleanly.
 *
 * Usage:
 *   ./monitor/build/monitor [--auto-resolve] [--features-path <path>]
 *
 *   --auto-resolve        Enable Phase 7 automatic SIGKILL on cycle detection.
 *                         Without this flag the monitor only logs + alerts.
 *   --features-path PATH  Where to write features.json (default: status/features.json)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include "graph.h"
#include "cycle_detector.h"
#include "ipc_reader.h"
#include "resolver.h"
#include "feature_extractor.h"
#include "../../ipc/fifo_channel.h"
#include "../../ipc/event_protocol.h"

/* ── Configuration ───────────────────────────────────────────────────────── */
#define POLL_INTERVAL_MS   500          /* feature extraction cadence        */
#define FEATURES_PATH_DEFAULT "status/features.json"
#define HEARTBEAT_PATH    "status/heartbeat.json"

/* ── Globals ─────────────────────────────────────────────────────────────── */
static graph_t        g;
static volatile int   running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

/* ── Heartbeat writer ────────────────────────────────────────────────────── */
static void write_heartbeat(int tick, const char *status) {
    FILE *f = fopen(HEARTBEAT_PATH, "w");
    if (!f) return;
    fprintf(f, "{\"tick\": %d, \"status\": \"%s\"}\n", tick, status);
    fclose(f);
}

/* ── Millisecond timer helper ────────────────────────────────────────────── */
static double ms_since(struct timespec *prev) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double ms = (now.tv_sec  - prev->tv_sec)  * 1000.0
              + (now.tv_nsec - prev->tv_nsec) / 1e6;
    *prev = now;
    return ms;
}

/* ── Main ────────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    /* Parse flags */
    int auto_resolve = 0;
    const char *features_path = FEATURES_PATH_DEFAULT;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--auto-resolve") == 0) {
            auto_resolve = 1;
        } else if (strcmp(argv[i], "--features-path") == 0 && i + 1 < argc) {
            features_path = argv[++i];
        }
    }

    fprintf(stderr, "[monitor] starting (auto-resolve=%s, features=%s)\n",
            auto_resolve ? "ON" : "OFF", features_path);

    /* Signal handling */
    signal(SIGINT,  handle_sigint);
    signal(SIGTERM, handle_sigint);

    /* Initialise graph and reader state */
    graph_init(&g);
    ipc_reader_reset();

    /* Open FIFO — blocks until first worker opens its write end */
    fprintf(stderr, "[monitor] waiting for workers on FIFO %s ...\n", FIFO_PATH);
    int fifo_fd = fifo_open_read();
    if (fifo_fd < 0) {
        fprintf(stderr, "[monitor] failed to open FIFO\n");
        return 1;
    }
    fprintf(stderr, "[monitor] FIFO open — event loop running\n");

    /* Set FIFO read to non-blocking so we can also poll the trigger file */
    int flags = fcntl(fifo_fd, F_GETFL, 0);
    fcntl(fifo_fd, F_SETFL, flags | O_NONBLOCK);

    /* Feature extraction timing */
    struct timespec last_feature_ts;
    clock_gettime(CLOCK_MONOTONIC, &last_feature_ts);
    int prev_wait_edges = 0;
    int tick = 0;

    write_heartbeat(tick, "running");

    while (running) {
        /* ── 1. Read one event (non-blocking) ── */
        ipc_event_t ev;
        int rc = fifo_recv_event(fifo_fd, &ev);

        if (rc == 1) {
            /* Got an event — apply to graph */
            ipc_reader_apply(&g, &ev);

            /* ── 2. Cycle detection after every WAIT edge ── */
            if (ev.type == EV_WAIT) {
                cycle_result_t result;
                if (detect_cycle(&g, &result)) {
                    fprintf(stderr, "\n[monitor] 🔴 ");
                    print_cycle(&g, &result);
                    write_heartbeat(tick, "deadlock");

                    if (auto_resolve) {
                        resolve_auto(&g, &result);
                        write_heartbeat(tick, "resolved");
                    }
                }
            }
        } else if (rc == 0) {
            /* EOF — all workers exited */
            fprintf(stderr, "[monitor] FIFO EOF — all workers done\n");
            break;
        }
        /* rc == -1 with EAGAIN/EWOULDBLOCK: no data yet, continue polling */

        /* ── 3. Manual trigger check (Phase 7) ── */
        if (check_manual_trigger()) {
            resolve_manual(&g);
            write_heartbeat(tick, "manually_resolved");
        }

        /* ── 4. Feature extraction every POLL_INTERVAL_MS ── */
        double elapsed_ms = ms_since(&last_feature_ts);
        if (elapsed_ms >= POLL_INTERVAL_MS) {
            features_t f = extract_features(&g, prev_wait_edges,
                                            elapsed_ms / 1000.0);
            write_features_json(&f, features_path);
            prev_wait_edges = f.edge_count;   /* approximation for growth rate */
            tick++;
            write_heartbeat(tick, "running");
            clock_gettime(CLOCK_MONOTONIC, &last_feature_ts);
        }

        /* Tiny sleep to avoid busy-spinning when no events arrive */
        usleep(5000);  /* 5 ms */
    }

    /* Cleanup */
    close(fifo_fd);
    fifo_destroy();
    write_heartbeat(tick, "stopped");
    fprintf(stderr, "[monitor] exiting\n");
    return 0;
}
