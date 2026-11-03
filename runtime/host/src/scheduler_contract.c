#include "scheduler_contract.h"

#define OS_CONTEXT_STATE_EXCEPTION 0x0002u
#define OS_SAVE_CONTEXT_CONTINUATION 0x80307EACu

static bool preserve_materialized_gpr(CPUState* cpu, u32 context,
                                      u32 reg) {
    return reg == 3u && cpu->pc == OS_SAVE_CONTEXT_CONTINUATION &&
           (mem_read16(cpu, context + 0x1A2u) & OS_CONTEXT_STATE_EXCEPTION) == 0u &&
           mem_read32(cpu, context + 0x0Cu) == 1u && cpu->gpr[3] == 0u;
}

void bluewake_scheduler_save_exception_context(CPUState* cpu, u32 context) {
    for (u32 reg = 0; reg < 32u; reg++) {
        if (!preserve_materialized_gpr(cpu, context, reg))
            mem_write32(cpu, context + reg * 4u, cpu->gpr[reg]);
    }
    mem_write32(cpu, context + 0x80u, cpu->cr);
    mem_write32(cpu, context + 0x84u, cpu->lr);
    mem_write32(cpu, context + 0x88u, cpu->ctr);
    mem_write32(cpu, context + 0x8Cu, cpu->xer);
    mem_write32(cpu, context + 0x198u, cpu->pc);
    mem_write32(cpu, context + 0x19Cu, cpu->msr);
    mem_write16(cpu, context + 0x1A2u,
                mem_read16(cpu, context + 0x1A2u) |
                    OS_CONTEXT_STATE_EXCEPTION);
    for (u32 reg = 0; reg < 8u; reg++)
        mem_write32(cpu, context + 0x1A4u + reg * 4u, cpu->gqr[reg]);
}

bool bluewake_scheduler_interrupt_safe(u32 pc) {
    // __OSReschedule -> SelectThread, the external-interrupt unwind, and
    // OSLoadContext each expose partial guest context across dispatch returns.
    return pc != 0x80307FD0u && pc != 0x80304DF8u &&
           !(pc >= 0x80303A50u && pc <= 0x80303B24u);
}

bool bluewake_scheduler_interrupt_requires_host(u32 pc, u32 msr,
                                                bool external_pending) {
    return external_pending && (msr & PPC_MSR_EE) != 0u &&
           bluewake_scheduler_interrupt_safe(pc);
}
