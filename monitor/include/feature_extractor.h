/*
 * monitor/include/feature_extractor.h — Phase 8/10 stub
 *
 * Declares the feature snapshot written to status/features.json so the
 * ML predictor can read it. Defined here so monitor/main.c can call it
 * from Phase 8 onward without touching main.c again.
 */

#pragma once

#include "graph.h"

/*
 * features_t — the four live metrics polled by the predictor.
 * Kept as a plain struct so it can be JSON-serialised trivially.
 */
typedef struct {
    int    blocked_count;     /* number of processes with at least one WAIT edge */
    double wait_time_growth;  /* rate of new WAIT edges per second (rolling avg) */
    int    edge_count;        /* total edges in the graph right now              */
    double graph_density;     /* edge_count / (MAX_NODES * (MAX_NODES-1))        */
} features_t;

/*
 * extract_features() — compute the current feature snapshot from the graph.
 * wait_edges_prev: edge count from the previous call (used for growth rate).
 * elapsed_seconds: time since the previous call.
 */
features_t extract_features(const graph_t *g,
                             int    wait_edges_prev,
                             double elapsed_seconds);

/*
 * write_features_json() — serialise features to status/features.json
 * so live_predict.py can read them.
 */
void write_features_json(const features_t *f, const char *path);
