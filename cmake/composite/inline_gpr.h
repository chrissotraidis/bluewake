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
