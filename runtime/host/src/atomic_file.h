// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BLUEWAKE_ATOMIC_FILE_H
#define BLUEWAKE_ATOMIC_FILE_H
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <process.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
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
static inline bool bw_atomic_flush_path(const char* path) {
    FILE* file = fopen(path, "r+b");
    if (!file) return false;
    bool ok = bw_atomic_flush(file);
    if (fclose(file) != 0) ok = false;
    return ok;
}
// Startup migration retains the source and never replaces an existing target.
static inline bool bw_atomic_copy_if_missing(const char* from, const char* to) {
    struct stat status;
    if (stat(to, &status) == 0) return true;
    if (errno != ENOENT) return false;
    FILE* source = fopen(from, "rb");
    if (source == NULL) return errno == ENOENT;
    char* pending = bw_atomic_path(to);
    FILE* target = pending ? fopen(pending, "wb") : NULL;
    bool ok = target != NULL;
    char buffer[8192]; size_t count;
    while (ok && (count = fread(buffer, 1, sizeof buffer, source)) != 0)
        ok = fwrite(buffer, 1, count, target) == count;
    if (ferror(source)) ok = false;
    if (fclose(source) != 0) ok = false;
    if (target) {
        if (ok && !bw_atomic_flush(target)) ok = false;
        if (fclose(target) != 0) ok = false;
        // A target can appear after stat(), before this process holds a card
        // lock. Publish without replacement so migration cannot clobber it.
        if (ok) {
#ifdef _WIN32
            if (!MoveFileExA(pending, to, MOVEFILE_WRITE_THROUGH)) {
                DWORD error = GetLastError();
                ok = error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS;
            }
#else
            ok = link(pending, to) == 0 || errno == EEXIST;
#endif
        }
        remove(pending);
    }
    free(pending);
    return ok;
}
#endif
