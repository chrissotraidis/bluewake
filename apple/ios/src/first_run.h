// First-run screen (first_run.m).
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// True when the app container lacks the composite, the disc or the files
// prepared from it.
bool bluewake_first_run_needed(const char* data_dir, const char* composite_path);

// Shows the first-run screen and returns once everything is in place and the
// player chose to start. Call on the main thread.
void bluewake_first_run_present(const char* data_dir, const char* composite_path);

#ifdef __cplusplus
}
#endif
