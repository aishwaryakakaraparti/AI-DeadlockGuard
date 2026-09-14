/*
 * monitor/src/feature_extractor.c — Phase 8/10 stub
 *
 * Computes live features from the wait-for graph and writes them to
 * status/features.json for the Python predictor to consume.
 */

#include "feature_extractor.h"
#include <stdio.h>
#include <string.h>

features_t extract_features(const graph_t *g,
                             int    wait_edges_prev,
                             double elapsed_seconds) {
    features_t f;
    memset(&f, 0, sizeof(f));

    int wait_edges = 0;

    /* Count blocked processes (thread nodes with ≥1 outgoing WAIT edge)
     * and total wait edges */
    for (int t = 0; t < MAX_THREADS; t++) {
        if (!g->active[t]) continue;
        int waiting = 0;
        for (int r = MAX_THREADS; r < MAX_NODES; r++) {
            if (g->adj[t][r]) {
                wait_edges++;
                waiting = 1;
            }
        }
        if (waiting) f.blocked_count++;
    }

    /* Count hold edges (resource → thread) for total edge_count */
    int hold_edges = 0;
    for (int r = MAX_THREADS; r < MAX_NODES; r++) {
        if (!g->active[r]) continue;
        for (int t = 0; t < MAX_THREADS; t++) {
            if (g->adj[r][t]) hold_edges++;
        }
    }

    f.edge_count = wait_edges + hold_edges;

    /* Wait-time growth: delta wait edges per second */
    int delta = wait_edges - wait_edges_prev;
    f.wait_time_growth = (elapsed_seconds > 0.0)
                         ? (double)delta / elapsed_seconds
                         : 0.0;

    /* Graph density: edges / max possible edges */
    double max_edges = (double)MAX_NODES * (MAX_NODES - 1);
    f.graph_density = (max_edges > 0.0)
                      ? (double)f.edge_count / max_edges
                      : 0.0;

    return f;
}

void write_features_json(const features_t *f, const char *path) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("write_features_json: fopen");
        return;
    }
    fprintf(fp,
        "{\n"
        "  \"blocked_count\": %d,\n"
        "  \"wait_time_growth\": %.4f,\n"
        "  \"edge_count\": %d,\n"
        "  \"graph_density\": %.6f\n"
        "}\n",
        f->blocked_count,
        f->wait_time_growth,
        f->edge_count,
        f->graph_density);
    fclose(fp);
}
