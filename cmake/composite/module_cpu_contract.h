#ifndef BLUEWAKE_MODULE_CPU_CONTRACT_H
#define BLUEWAKE_MODULE_CPU_CONTRACT_H

#include "StaticRecompABI.h"
#include <stdint.h>
#include <string.h>

/* BlueWake's fixed-CPU variant keeps the v3 descriptor layout and requires
 * bluewake_composite_guest_cpu(). Older hosts reject its distinct ABI before
 * executing code that ignores their CPU pointer. Ordinary modules stay v3.
 * Revisit this extension explicitly if the upstream descriptor ABI changes. */
#if STATICRECOMP_ABI_VERSION != 3u
#error "Review BlueWake fixed-CPU compatibility for the new module ABI"
#endif
#define BLUEWAKE_FIXED_CPU_ABI_VERSION 4u

typedef CPUState* (*BlueWakeModuleCPUFn)(void);

/* Validate before calling a module getter or initializing graphics/player data.
 * The selected CPU is borrowed; cpu_free releases its allocations, not itself.
 * Global MEM1 needs a separate ownership contract and is deliberately rejected
 * until the host can adopt and release that storage correctly. */
static inline const char* bw_module_select_cpu(
    const StaticRecompModuleDesc* module, BlueWakeModuleCPUFn getter,
    int has_global_mem1, CPUState* fallback, CPUState** selected) {
    *selected = NULL;
    if (module == NULL || module->dispatch == NULL)
        return "missing module descriptor or dispatch function";
    if (module->abi_version != STATICRECOMP_ABI_VERSION &&
        module->abi_version != BLUEWAKE_FIXED_CPU_ABI_VERSION)
        return "unsupported module ABI; rebuild the module for this app";
    if (module->cpu_abi_version != GXRUNTIME_CPU_ABI_VERSION ||
        module->cpu_state_size != sizeof(CPUState))
        return "CPU layout mismatch; rebuild the module for this app";
    if (memcmp(module->game_id, "GZLE01", sizeof("GZLE01")) != 0)
        return "unsupported module game identity (expected GZLE01)";
    if (has_global_mem1)
        return "global-MEM1 module needs a supporting host; rebuild without it";
    if (module->abi_version == STATICRECOMP_ABI_VERSION && getter != NULL)
        return "legacy fixed-CPU module lacks its ABI declaration; rebuild it";
    if (module->abi_version == BLUEWAKE_FIXED_CPU_ABI_VERSION && getter == NULL)
        return "fixed-CPU module is missing its required CPU getter";
    CPUState* state = getter != NULL ? getter() : fallback;
#ifdef __cplusplus
    const size_t alignment = alignof(CPUState);
#else
    const size_t alignment = _Alignof(CPUState);
#endif
    if (state == NULL || (uintptr_t)state % alignment != 0)
        return "module CPU storage is null or misaligned";
    *selected = state;
    return NULL;
}

#endif
