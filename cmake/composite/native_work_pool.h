#ifndef BLUEWAKE_NATIVE_WORK_POOL_H
#define BLUEWAKE_NATIVE_WORK_POOL_H
#include <stddef.h>

// Synchronous fork/join for pure, independent ranges. The caller owns the
// inputs until return; callbacks must not access guest CPU/device/global state.
typedef void (*BluewakeRangeWork)(void* context, size_t first, size_t end);
void bluewake_parallel_range(size_t count, size_t minimum_grain,
                             BluewakeRangeWork work, void* context);
unsigned long long bluewake_parallel_batches(void);
typedef struct BluewakePoolStats {
    unsigned requested_workers;
    unsigned effective_workers;
    unsigned long long completed_batches;
    unsigned long long worker_items;
} BluewakePoolStats;
void bluewake_parallel_stats(BluewakePoolStats *out);
#if defined(BLUEWAKE_POOL_TESTING)
unsigned bluewake_parallel_workers(void);
#endif
#endif
