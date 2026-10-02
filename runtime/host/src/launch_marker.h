// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BLUEWAKE_LAUNCH_MARKER_H
#define BLUEWAKE_LAUNCH_MARKER_H
#include "atomic_file.h"
#include <time.h>
static inline bool bw_launch_pending(const char* marker) {
    struct stat status;
    return stat(marker, &status) == 0;
}
static inline bool bw_launch_backup(const char* settings) {
    size_t capacity = strlen(settings) + 80;
    char* backup = (char*)malloc(capacity);
    if (!backup) return false;
    snprintf(backup, capacity, "%s.before-safe-mode-%lld-%lu", settings,
        (long long)time(NULL), (unsigned long)getpid());
    bool ok = bw_atomic_copy_if_missing(settings, backup);
    free(backup);
    return ok;
}
static inline bool bw_launch_begin(const char* marker) {
    char* pending = bw_atomic_path(marker);
    FILE* file = pending ? fopen(pending, "wb") : NULL;
    bool ok = file && fputs("BlueWake launch pending\n", file) >= 0;
    if (file) ok = bw_atomic_finish(file, pending, marker, ok);
    free(pending);
    return ok;
}
static inline bool bw_launch_clear(const char* marker) {
    return remove(marker) == 0 || errno == ENOENT;
}
#endif
