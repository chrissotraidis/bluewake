#ifndef BLUEWAKE_MODULE_HUGEPAGES_H
#define BLUEWAKE_MODULE_HUGEPAGES_H

/* The game module's code on 2 MB pages (#554 M2; opt-in, BLUEWAKE_MODULE_HUGEPAGES=1).
 *
 * The translated module's code is hundreds of megabytes, and the game thread
 * runs a few tens of megabytes of it scattered through the rest: on 4 KB pages
 * the instruction TLB misses about 0.4 times per 1,000 instructions, each a
 * page-table walk. On the Steam Deck the module comes from the AppImage's FUSE
 * mount, so the kernel cannot back it with file huge pages. After dlopen, and
 * before any of its code runs, this copies the 2 MB-aligned middle of the
 * module's executable segment into anonymous memory the kernel may back with
 * huge pages (transparent huge pages, "always" or "madvise"), at the same
 * addresses and with the same bytes: relocations are already applied
 * (RTLD_NOW) and the code is position-independent, so nothing it does changes.
 *
 * Costs: the copied range is resident anonymous memory instead of page cache,
 * and a profiler no longer maps those addresses to the module file (perf shows
 * them unsymbolized), which is one reason it is off by default. Linux only. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__)
#include <limits.h>
#include <string.h>
#include <sys/mman.h>

/* The module's executable mapping, from /proc/self/maps by its file's real
 * path (no GNU dl extensions: main.c includes the system headers before any
 * feature macro). */
static bool bluewake_module_text_find(const char* module_path, uintptr_t* start, uintptr_t* end) {
    char real[PATH_MAX];
    if (realpath(module_path, real) == NULL)
        return false;
    FILE* maps = fopen("/proc/self/maps", "r");
    if (maps == NULL)
        return false;
    char line[PATH_MAX + 128];
    bool found = false;
    while (!found && fgets(line, sizeof line, maps) != NULL) {
        unsigned long from = 0, to = 0;
        char perms[8] = {0};
        int path_at = 0;
        if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %n", &from, &to, perms, &path_at) < 3 || path_at == 0)
            continue;
        char* path = line + path_at;
        path[strcspn(path, "\n")] = '\0';
        if (strcmp(perms, "r-xp") == 0 && strcmp(path, real) == 0) {
            *start = (uintptr_t)from;
            *end = (uintptr_t)to;
            found = true;
        }
    }
    fclose(maps);
    return found;
}

/* Returns true when the range was moved; *moved_bytes is its size. A failure
 * before the range is replaced leaves the module as it was. */
static bool bluewake_module_text_hugepages(const char* module_path, uintptr_t* moved_start,
                                           size_t* moved_bytes, const char** why) {
    *moved_start = 0;
    *moved_bytes = 0;
    uintptr_t text_start = 0, text_end = 0;
    if (!bluewake_module_text_find(module_path, &text_start, &text_end) || text_end <= text_start) {
        *why = "the module's executable mapping is not found";
        return false;
    }
    const uintptr_t huge = 2u * 1024u * 1024u;
    const uintptr_t start = (text_start + huge - 1u) & ~(huge - 1u);
    const uintptr_t end = text_end & ~(huge - 1u);
    if (end <= start) {
        *why = "the executable segment holds no whole 2 MB page";
        return false;
    }
    const size_t length = end - start;
    void* copy = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (copy == MAP_FAILED) {
        *why = "no memory for the copy";
        return false;
    }
    memcpy(copy, (const void*)start, length);
    /* From here the module's code in the range is the copy's: a failure is fatal. */
    void* moved = mmap((void*)start, length, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (moved != (void*)start) {
        fprintf(stderr, "[module] huge pages: could not map the code range back; stopping\n");
        abort();
    }
    (void)madvise(moved, length, MADV_HUGEPAGE); /* EINVAL without THP: 4 KB pages, still correct */
    memcpy(moved, copy, length);
    if (mprotect(moved, length, PROT_READ | PROT_EXEC) != 0) {
        fprintf(stderr, "[module] huge pages: could not make the code range executable; stopping\n");
        abort();
    }
    munmap(copy, length);
    *moved_start = start;
    *moved_bytes = length;
    return true;
}

/* AnonHugePages of the mapping that starts at `start`, from /proc/self/smaps:
 * how much of the range the kernel actually backs with huge pages. */
static size_t bluewake_module_text_huge_kb(const void* start) {
    FILE* smaps = fopen("/proc/self/smaps", "r");
    if (smaps == NULL)
        return 0;
    char line[512];
    bool in_range = false;
    size_t kb = 0;
    while (fgets(line, sizeof line, smaps) != NULL) {
        unsigned long from = 0, to = 0;
        if (sscanf(line, "%lx-%lx ", &from, &to) == 2) {
            in_range = (uintptr_t)from == (uintptr_t)start;
            continue;
        }
        if (in_range && strncmp(line, "AnonHugePages:", 14) == 0) {
            sscanf(line + 14, "%zu", &kb);
            break;
        }
    }
    fclose(smaps);
    return kb;
}
#endif

#endif
