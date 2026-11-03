#include "fpu_context.h"

#define OS_CURRENT_CONTEXT 0x800000D4u
#define OS_CURRENT_FPU_CONTEXT 0x800000D8u
#define OS_CONTEXT_STATE 0x1A2u
#define OS_CONTEXT_SIZE 0x2C8u
#define OS_CONTEXT_STATE_FPSAVED 0x0001u
#define OS_CONTEXT_STATE_EXC 0x0002u
#define OS_FPU_EXCEPTION_HANDLER 0x80303EB8u

static bool valid_context(const CPUState* cpu, u32 context) {
    return cpu != NULL && context >= GC_RAM_BASE &&
           OS_CONTEXT_SIZE <= cpu->ram_size &&
           context - GC_RAM_BASE <= cpu->ram_size - OS_CONTEXT_SIZE;
}

static void save_exception_image(CPUState* cpu, u32 context) {
    for (u32 reg = 0; reg < 32u; reg++)
        mem_write32(cpu, context + reg * 4u, cpu->gpr[reg]);
    mem_write32(cpu, context + 0x080u, cpu->cr);
    mem_write32(cpu, context + 0x084u, cpu->lr);
    mem_write32(cpu, context + 0x088u, cpu->ctr);
    mem_write32(cpu, context + 0x08Cu, cpu->xer);
    mem_write32(cpu, context + 0x198u, cpu->srr0);
    mem_write32(cpu, context + 0x19Cu, cpu->srr1);
    mem_write16(cpu, context + OS_CONTEXT_STATE,
                mem_read16(cpu, context + OS_CONTEXT_STATE) |
                    OS_CONTEXT_STATE_EXC);
    for (u32 reg = 0; reg < 8u; reg++)
        mem_write32(cpu, context + 0x1A4u + reg * 4u, cpu->gqr[reg]);
}

BluewakeFpuSwitchResult bluewake_fpu_exception_deliver(CPUState* cpu) {
    if (cpu == NULL)
        return BLUEWAKE_FPU_SWITCH_INVALID;

    const u32 current = mem_read32(cpu, OS_CURRENT_CONTEXT);
    const u32 previous = mem_read32(cpu, OS_CURRENT_FPU_CONTEXT);
    if (!valid_context(cpu, current) ||
        (previous != 0u && !valid_context(cpu, previous)))
        return BLUEWAKE_FPU_SWITCH_INVALID;

    BluewakeFpuSwitchResult result = BLUEWAKE_FPU_SWITCH_SAME_OWNER;
    if (previous != current) {
        result = (mem_read16(cpu, current + OS_CONTEXT_STATE) &
                  OS_CONTEXT_STATE_FPSAVED) != 0u
                     ? BLUEWAKE_FPU_SWITCH_RESTORED
                     : BLUEWAKE_FPU_SWITCH_FRESH;
    }

    save_exception_image(cpu, current);
    cpu->exception = 0u;
    cpu->gpr[4] = current;
    cpu->pc = OS_FPU_EXCEPTION_HANDLER;
    return result;
}

bool bluewake_fpu_registers_materialized(CPUState* cpu) {
    if (cpu == NULL || (cpu->msr & PPC_MSR_FP) == 0u)
        return false;
    const u32 current = mem_read32(cpu, OS_CURRENT_CONTEXT);
    return valid_context(cpu, current) &&
           mem_read32(cpu, OS_CURRENT_FPU_CONTEXT) == current;
}
