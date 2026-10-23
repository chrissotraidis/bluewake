#ifndef BLUEWAKE_INLINE_GPR_H
#define BLUEWAKE_INLINE_GPR_H

#include "direct_calls.h"

/* Bulk charging cannot expose a different PC/downcount to an MMIO or journal
 * callback. Check each word: a partial alias inside the span would be missed
 * by resolving the whole frame as one range. Actual stores still use the
 * maintained memory helpers, including reservation invalidation. */
static inline bool bw_inline_gpr_memory_ready(CPUState* cpu, unsigned first) {
    if (first < 14 || first > 31 || cpu->ram == NULL || g_mem_write_journal != NULL)
        return false;
    /* The save/restore frame is one aligned, contiguous range.  With no
     * alias overlapping MEM1, checking its first byte and total size proves
     * every word resolves to the ordinary RAM backing.  Keep the per-word
     * resolver below for the uncommon case where an alias may cover only
     * part of the frame. */
    if (!g_ppc_guest_aliases_overlap_mem1) {
        const u32 size = 4u * (32u - first);
        const u32 address = cpu->gpr[11] - size;
        const u32 offset = (address & ~0x40000000u) - GC_RAM_BASE;
        return (address & 3u) == 0u && size <= cpu->ram_size &&
               offset <= cpu->ram_size - size &&
               get_ram_ptr(cpu, address, size, NULL) == cpu->ram + offset;
    }
    for (unsigned reg = first; reg < 32; ++reg) {
        const u32 address = cpu->gpr[11] + (u32)(4 * reg - 128);
        const u32 offset = (address & ~0x40000000u) - GC_RAM_BASE;
        if ((address & 3u) != 0u || cpu->ram_size < 4u || offset > cpu->ram_size - 4u ||
            get_ram_ptr(cpu, address, 4u, NULL) != cpu->ram + offset)
            return false;
    }
    return true;
}

#endif
