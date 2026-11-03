#ifndef BLUEWAKE_MODULE_CPU_CONTRACT_H
#define BLUEWAKE_MODULE_CPU_CONTRACT_H

#include "StaticRecompABI.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* BlueWake's storage variants keep the v3 descriptor layout: v4 requires
 * bluewake_composite_guest_cpu(), v5 additionally requires the MEM1 getter.
 * Older hosts reject these ABIs before executing code on different storage.
 * Ordinary modules stay v3.
 * Revisit this extension explicitly if the upstream descriptor ABI changes. */
#if STATICRECOMP_ABI_VERSION != 3u
#error "Review BlueWake fixed-CPU compatibility for the new module ABI"
#endif
#define BLUEWAKE_FIXED_CPU_ABI_VERSION 4u
#define BLUEWAKE_FIXED_MEM1_ABI_VERSION 5u
#define BLUEWAKE_MODULE_MEM1_SIZE 0x02000000u

typedef CPUState* (*BlueWakeModuleCPUFn)(void);
typedef u8* (*BlueWakeModuleMEM1Fn)(u32* size);
typedef struct BlueWakeModuleStorage {
    CPUState* cpu;
    u8* mem1;
    u32 mem1_size;
} BlueWakeModuleStorage;

/* Validate before calling a module getter or initializing graphics/player data.
 * CPU and optional MEM1 storage are borrowed. ABI 5 declares both required
 * getters so a missing export cannot silently select a different RAM buffer. */
static inline const char* bw_module_select_storage(
    const StaticRecompModuleDesc* module, BlueWakeModuleCPUFn getter,
    BlueWakeModuleMEM1Fn mem1_getter, CPUState* fallback,
    BlueWakeModuleStorage* selected) {
    selected->cpu = NULL;
    selected->mem1 = NULL;
    selected->mem1_size = 0;
    if (module == NULL || module->dispatch == NULL)
        return "missing module descriptor or dispatch function";
    if (module->abi_version != STATICRECOMP_ABI_VERSION &&
        module->abi_version != BLUEWAKE_FIXED_CPU_ABI_VERSION &&
        module->abi_version != BLUEWAKE_FIXED_MEM1_ABI_VERSION)
        return "unsupported module ABI; rebuild the module for this app";
    if (module->cpu_abi_version != GXRUNTIME_CPU_ABI_VERSION ||
        module->cpu_state_size != sizeof(CPUState))
        return "CPU layout mismatch; rebuild the module for this app";
    if (memcmp(module->game_id, "GZLE01", sizeof("GZLE01")) != 0)
        return "unsupported module game identity (expected GZLE01)";
    if (module->abi_version == STATICRECOMP_ABI_VERSION && getter != NULL)
        return "legacy fixed-CPU module lacks its ABI declaration; rebuild it";
    if (module->abi_version != STATICRECOMP_ABI_VERSION && getter == NULL)
        return "fixed-CPU module is missing its required CPU getter";
    if (module->abi_version == BLUEWAKE_FIXED_MEM1_ABI_VERSION && mem1_getter == NULL)
        return "fixed-MEM1 module is missing its required memory getter";
    if (module->abi_version != BLUEWAKE_FIXED_MEM1_ABI_VERSION && mem1_getter != NULL)
        return "legacy fixed-MEM1 module lacks its ABI declaration; rebuild it";
    CPUState* state = getter != NULL ? getter() : fallback;
#ifdef __cplusplus
    const size_t alignment = alignof(CPUState);
#else
    const size_t alignment = _Alignof(CPUState);
#endif
    if (state == NULL || (uintptr_t)state % alignment != 0)
        return "module CPU storage is null or misaligned";
    u32 mem1_size = 0;
    u8* mem1 = mem1_getter != NULL ? mem1_getter(&mem1_size) : NULL;
    if (mem1_getter != NULL) {
        if (mem1 == NULL || (uintptr_t)mem1 % 16u != 0 ||
            mem1_size != BLUEWAKE_MODULE_MEM1_SIZE)
            return "module MEM1 must be aligned storage of exactly 32 MiB";
        const uintptr_t ram_start = (uintptr_t)mem1, cpu_start = (uintptr_t)state;
        if (ram_start > UINTPTR_MAX - mem1_size ||
            cpu_start > UINTPTR_MAX - sizeof(CPUState) ||
            (ram_start < cpu_start + sizeof(CPUState) && cpu_start < ram_start + mem1_size))
            return "module CPU and MEM1 storage must not overlap";
    }
    selected->cpu = state;
    selected->mem1 = mem1;
    selected->mem1_size = mem1_size;
    return NULL;
}

static inline bool bw_module_cpu_init(const BlueWakeModuleStorage* storage) {
    if (!cpu_init(storage->cpu)) return false;
    if (storage->mem1 != NULL) {
        free(storage->cpu->ram);
        memset(storage->mem1, 0, storage->mem1_size);
        storage->cpu->ram = storage->mem1;
        storage->cpu->ram_size = storage->mem1_size;
    }
    return true;
}

static inline void bw_module_cpu_free(const BlueWakeModuleStorage* storage) {
    if (storage->mem1 != NULL && storage->cpu->ram == storage->mem1)
        storage->cpu->ram = NULL;
    cpu_free(storage->cpu);
}

#endif
