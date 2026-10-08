#ifndef BLUEWAKE_CACHE_FLUSH_FALLBACK_H
#define BLUEWAKE_CACHE_FLUSH_FALLBACK_H

#include "core/cpu.h"

// C-generated cache instructions reach the host fallback. Forward writeback
// operations to the same observer used by LLVM-generated instructions.
static inline bool bluewake_cache_flush_fallback(CPUState* cpu, u32 raw, u32 cia) {
    const u32 xo = (raw >> 1) & 0x3FFu;
    if ((raw >> 26) != 31u || (xo != 54u && xo != 86u))
        return false;
    const u32 ra = (raw >> 16) & 31u;
    const u32 rb = (raw >> 11) & 31u;
    const u32 ea = (ra == 0u ? 0u : cpu->gpr[ra]) + cpu->gpr[rb];
    ppc_cache_control(cpu, xo == 54u ? PPC_CACHE_DCBST : PPC_CACHE_DCBF, ea, cia);
    if (!cpu->exception)
        cpu->pc = cia + 4u;
    return true;
}

#endif
