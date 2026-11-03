#include "scheduler_contract.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

static void seed_cpu(CPUState* cpu) {
    for (u32 reg = 0; reg < 32u; reg++)
        cpu->gpr[reg] = 0xA0000000u + reg;
    cpu->cr = 0x12345678u;
    cpu->lr = 0x81234560u;
    cpu->ctr = 0x82345670u;
    cpu->xer = 0x87654321u;
    cpu->msr = PPC_MSR_EE | PPC_MSR_RI;
    cpu->fpscr = 0xA5A5A5A5u;
    cpu->gqr[2] = 0x02020002u;
    cpu->fpr[7] = 7.25;
}

int main(void) {
    CPUState cpu;
    assert(cpu_init(&cpu));
    const u32 context_a = GC_RAM_BASE + 0x400u;
    const u32 context_b = GC_RAM_BASE + 0x800u;

    seed_cpu(&cpu);
    cpu.pc = 0x80307EACu;
    cpu.gpr[3] = 0u;
    mem_write16(&cpu, context_a + 0x1A2u, 0u);
    mem_write32(&cpu, context_a + 0x0Cu, 1u);
    mem_write64(&cpu, context_a + 0x90u + 7u * 8u,
                0x1122334455667788ull);
    mem_write32(&cpu, context_a + 0x194u, 0x89ABCDEFu);
    bluewake_scheduler_save_exception_context(&cpu, context_a);
    assert(mem_read32(&cpu, context_a + 0x0Cu) == 1u);
    assert(mem_read32(&cpu, context_a + 0x10u) == cpu.gpr[4]);
    assert(mem_read32(&cpu, context_a + 0x198u) == 0x80307EACu);
    assert(mem_read16(&cpu, context_a + 0x1A2u) == 0x0002u);
    assert(mem_read64(&cpu, context_a + 0x90u + 7u * 8u) ==
           0x1122334455667788ull);
    assert(mem_read32(&cpu, context_a + 0x194u) == 0x89ABCDEFu);

    seed_cpu(&cpu);
    cpu.pc = 0x8027B66Cu;
    cpu.gpr[3] = 0u;
    mem_write16(&cpu, context_b + 0x1A2u, 0x0001u);
    mem_write32(&cpu, context_b + 0x0Cu, 1u);
    bluewake_scheduler_save_exception_context(&cpu, context_b);
    assert(mem_read32(&cpu, context_b + 0x0Cu) == 0u);
    assert(mem_read16(&cpu, context_b + 0x1A2u) == 0x0003u);

    assert(!bluewake_scheduler_interrupt_safe(0x80307FD0u));
    assert(!bluewake_scheduler_interrupt_safe(0x80304DF8u));
    assert(!bluewake_scheduler_interrupt_safe(0x80303A50u));
    assert(!bluewake_scheduler_interrupt_safe(0x80303B24u));
    assert(bluewake_scheduler_interrupt_safe(0x8027B66Cu));
    assert(!bluewake_scheduler_interrupt_requires_host(
        0x8027B66Cu, PPC_MSR_EE, false));
    assert(!bluewake_scheduler_interrupt_requires_host(
        0x8027B66Cu, 0u, true));
    assert(!bluewake_scheduler_interrupt_requires_host(
        0x80307FD0u, PPC_MSR_EE, true));
    assert(bluewake_scheduler_interrupt_requires_host(
        0x8027B66Cu, PPC_MSR_EE, true));

    cpu_free(&cpu);
    puts("Scheduler context-coherence contract test passed.");
    return 0;
}
