#ifndef BLUEWAKE_NATIVE_WORK_POOL_H
#define BLUEWAKE_NATIVE_WORK_POOL_H
#include <stddef.h>

// Synchronous fork/join for pure, independent ranges. The caller owns the
// inputs until return; callbacks must not access guest CPU/device/global state.
typedef void (*BluewakeRangeWork)(void* context, size_t first, size_t end);
void bluewake_parallel_range(size_t count, size_t minimum_grain,
                             BluewakeRangeWork work, void* context);
unsigned long long bluewake_parallel_batches(void);
#endif
