// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
static int inject_race;
static void competing_target(const char* path) {
    if (!inject_race) return;
    FILE* file = fopen(path, "wb");
    assert(file && fputs("newer player data", file) >= 0 && fclose(file) == 0);
}
static int raced_rename(const char* from, const char* to) {
    competing_target(to);
    return rename(from, to);
}
#ifdef _WIN32
static BOOL raced_move(const char* from, const char* to, DWORD flags) {
    competing_target(to);
    return MoveFileExA(from, to, flags);
}
#define MoveFileExA raced_move
#else
static int raced_link(const char* from, const char* to) {
    competing_target(to);
    return link(from, to);
}
#define link raced_link
#endif
#define rename raced_rename
#include "atomic_file.h"
#undef rename
#ifdef _WIN32
#undef MoveFileExA
#else
#undef link
#endif
int main(void) {
    char from[256], to[256];
    snprintf(from, sizeof from, "copy-race-%lu-source", (unsigned long)getpid());
    snprintf(to, sizeof to, "copy-race-%lu-target", (unsigned long)getpid());
    FILE* file = fopen(from, "wb");
    assert(file && fputs("legacy data", file) >= 0 && fclose(file) == 0);
    inject_race = 1;
    assert(bw_atomic_copy_if_missing(from, to));
    char bytes[64] = {0};
    file = fopen(to, "rb");
    assert(file && fread(bytes, 1, sizeof bytes - 1, file) == strlen("newer player data"));
    assert(fclose(file) == 0 && strcmp(bytes, "newer player data") == 0);
    assert(remove(to) == 0);
    inject_race = 0;
    assert(bw_atomic_copy_if_missing(from, to));
    file = fopen(to, "rb");
    memset(bytes, 0, sizeof bytes);
    assert(file && fread(bytes, 1, sizeof bytes - 1, file) == strlen("legacy data"));
    assert(fclose(file) == 0 && strcmp(bytes, "legacy data") == 0);
    assert(remove(from) == 0 && remove(to) == 0);
    puts("atomic migration preserves a concurrently created target");
}
