// The first run's stage list. See disc_import_stages.h.
#include "disc_import_stages.h"

#include <stdio.h>
#include <string.h>

static const char* const kNames[BW_IMPORT_STAGE_COUNT] = {
    "Checking the disc image and the data folder",
    "Reading the disc",
    "Unpacking the game modules",
    "Saving the settings and the shortcuts",
};

const char* bw_import_stage_name(int stage) {
    return stage < 0 || stage >= BW_IMPORT_STAGE_COUNT ? NULL : kNames[stage];
}

int bw_import_stage_for_report(const char* report, double fraction) {
    if (report != NULL) {
        if (strcmp(report, "Reading the disc") == 0) return BW_IMPORT_STAGE_READ;
        if (strcmp(report, "Unpacking game modules") == 0) return BW_IMPORT_STAGE_UNPACK;
        // "Ready" arrives with fraction 1.0 once the outputs are swapped in.
        if (strcmp(report, "Ready") == 0) return BW_IMPORT_STAGE_UNPACK;
    }
    return fraction < 0.3 ? BW_IMPORT_STAGE_READ : BW_IMPORT_STAGE_UNPACK;
}

double bw_import_overall_fraction(int stage, double within) {
    static const double first[] = {0.00, 0.03, 0.10, 0.95};
    static const double last[] = {0.03, 0.10, 0.95, 1.00};
    if (stage < BW_IMPORT_STAGE_CHECK) stage = BW_IMPORT_STAGE_CHECK;
    if (stage >= BW_IMPORT_STAGE_COUNT) stage = BW_IMPORT_STAGE_COUNT - 1;
    if (!(within > 0.0)) within = 0.0;   // also catches NaN
    if (within > 1.0) within = 1.0;
    return first[stage] + (last[stage] - first[stage]) * within;
}

int bw_import_stage_state(int stage, int current, int failed, int finished) {
    if (failed && stage == current) return BW_IMPORT_FAILED;
    if (finished || stage < current) return BW_IMPORT_DONE;
    if (stage > current) return BW_IMPORT_PENDING;
    return BW_IMPORT_RUNNING;
}

const char* bw_import_stage_mark(int state) {
    switch (state) {
    case BW_IMPORT_DONE: return "[x]";
    case BW_IMPORT_RUNNING: return "[>]";
    case BW_IMPORT_FAILED: return "[!]";
    default: return "[ ]";
    }
}

size_t bw_import_stage_lines(int current, int failed, int finished, char* out, size_t size) {
    if (out == NULL || size == 0) return 0;
    out[0] = '\0';
    size_t used = 0;
    for (int stage = 0; stage < BW_IMPORT_STAGE_COUNT; ++stage) {
        const int state = bw_import_stage_state(stage, current, failed, finished);
        const int n = snprintf(out + used, used < size ? size - used : 0, "%s %s\n",
                               bw_import_stage_mark(state), kNames[stage]);
        if (n < 0) break;
        used += (size_t)n;
        if (used >= size) { used = size - 1; break; }
    }
    return used;
}
