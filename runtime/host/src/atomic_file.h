// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BLUEWAKE_ATOMIC_FILE_H
#define BLUEWAKE_ATOMIC_FILE_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <process.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
static inline char* bw_atomic_path(const char* path) {
    size_t capacity = strlen(path) + 40;
    char* pending = (char*)malloc(capacity);
    if (pending) snprintf(pending, capacity, "%s.tmp-%lu", path, (unsigned long)getpid());
    return pending;
}
static inline bool bw_atomic_flush(FILE* file) {
    if (ferror(file) || fflush(file) != 0) return false;
#ifdef _WIN32
    return _commit(_fileno(file)) == 0;
#elif defined(__APPLE__)
    return fcntl(fileno(file), F_FULLFSYNC) == 0;
#else
    return fsync(fileno(file)) == 0;
#endif
}
static inline bool bw_atomic_finish(FILE* file, const char* pending, const char* path, bool ok) {
    if (ok && !bw_atomic_flush(file)) ok = false;
    if (fclose(file) != 0) ok = false;
    if (ok && rename(pending, path) != 0) ok = false;
    if (!ok) remove(pending);
    return ok;
}
static inline bool bw_atomic_finish_dirty(FILE* file, const char* pending, const char* path, bool* dirty) {
    bool ok = bw_atomic_finish(file, pending, path, true);
    if (ok) *dirty = false;
    return ok;
}
#endif
