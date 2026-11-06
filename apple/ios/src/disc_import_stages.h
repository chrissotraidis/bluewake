// The stages every platform's first run shows while bluewake_disc_prepare()
// works: what they are called, which one an importer report belongs to, and the
// text lines a plain-text UI shows. Kept free of every GUI toolkit, so Linux's
// ImGui window, Windows' progress window and the iOS/tvOS first-run screens all
// name and order the work the same way, and a headless test can check it.
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    BW_IMPORT_STAGE_CHECK = 0,  // the disc image and the folder it prepares into
    BW_IMPORT_STAGE_READ,       // the disc header and main.dol
    BW_IMPORT_STAGE_UNPACK,     // the game modules, then swapped into place
    BW_IMPORT_STAGE_SAVE,       // settings, launcher shortcuts, the remembered disc
    BW_IMPORT_STAGE_COUNT
};

enum {
    BW_IMPORT_PENDING = 0,
    BW_IMPORT_RUNNING,
    BW_IMPORT_DONE,
    BW_IMPORT_FAILED
};

// "Checking the disc image and the data folder", ... ; NULL for an out-of-range
// stage.
const char* bw_import_stage_name(int stage);

// Which stage an importer report belongs to: by its own stage name when it has
// one ("Reading the disc", "Unpacking game modules", "Ready"), otherwise by the
// fraction it reports (below 0.3 is reading; from 0.3 the modules). Returns
// BW_IMPORT_STAGE_READ or BW_IMPORT_STAGE_UNPACK only: the checking and saving
// stages belong to the platform's UI, not to the importer.
int bw_import_stage_for_report(const char* report, double fraction);

// One overall 0..1 value for a progress bar: each stage owns a slice, so the
// bar keeps moving across all four instead of restarting per stage.
double bw_import_overall_fraction(int stage, double within);

// The state one stage is in, given the running stage (or the one that failed)
// and the job's own state.
int bw_import_stage_state(int stage, int current, int failed, int finished);

// The mark a text UI puts in front of a stage: "[x]" done, "[>]" running,
// "[ ]" pending, "[!]" failed.
const char* bw_import_stage_mark(int state);

// The four stages as lines for a label-style UI ("[x] Reading the disc"), and
// the same with a trailing sentence for the stage in progress. Returns the
// number of bytes written, like snprintf.
size_t bw_import_stage_lines(int current, int failed, int finished, char* out, size_t size);

#ifdef __cplusplus
}
#endif
