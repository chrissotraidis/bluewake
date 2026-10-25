#ifndef BLUEWAKE_NATIVE_MATH_H
#define BLUEWAKE_NATIVE_MATH_H
#include "core/cpu.h"
typedef bool (*BluewakeNativeMathReady)(void*, const CPUState*, u32);
typedef struct BluewakeNativeMathStatsV1 {
    u32 version;
    u32 size;
    u32 requested_workers;
    u32 effective_workers;
    u64 native_hits[4];
    u64 native_fallbacks[4];
    u64 array_vectors;
    u64 array_max_vectors;
    u64 completed_batches;
    u64 worker_items;
} BluewakeNativeMathStatsV1;
int bluewake_composite_native_math_v1(bool enabled, BluewakeNativeMathReady ready, void* user);
int bluewake_composite_native_math_stats_v1(BluewakeNativeMathStatsV1* out,
                                             u32 out_size);
int bluewake_native_math_try(CPUState* cpu, u32 address);
// Returns zero without changing state when exact replacement is unavailable.
int bluewake_native_math(CPUState* cpu, u32 address);
int bluewake_native_gpr(CPUState* cpu, u32 address);
void bluewake_native_math_report(void);
#endif
