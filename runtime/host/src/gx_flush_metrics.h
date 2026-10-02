// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BLUEWAKE_GX_FLUSH_METRICS_H
#define BLUEWAKE_GX_FLUSH_METRICS_H
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
static inline uint64_t bw_gx_flush_start(const char* text) {
    if (!text || !text[0] || text[0] == '-') return 13800;
    char* end; errno = 0;
    unsigned long long result = strtoull(text, &end, 10);
    return errno || *end ? 13800 : result;
}
static inline uint64_t bw_elapsed_us(struct timespec before, struct timespec after) {
    int64_t ns = (int64_t)(after.tv_sec - before.tv_sec) * 1000000000ll + after.tv_nsec - before.tv_nsec;
    return ns > 0 ? (uint64_t)ns / 1000u : 0;
}
#endif
