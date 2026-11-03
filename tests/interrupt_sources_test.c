#ifdef NDEBUG
#undef NDEBUG
#endif
#include "interrupt_sources.h"

#include <assert.h>
#include <string.h>

typedef struct Fixture {
    DolInterrupts interrupts;
    unsigned deadline_calls;
} Fixture;

static u64 deadline(const CPUState* cpu, void* user) {
    Fixture* fixture = user;
    fixture->deadline_calls++;
    return (cpu->msr & PPC_MSR_EE) != 0u &&
                   dol_interrupts_external_pending(&fixture->interrupts)
               ? 1u
               : UINT64_MAX;
}

int main(void) {
    CPUState cpu;
    Fixture fixture;
    BluewakeCycleDomain domain;
    memset(&cpu, 0, sizeof(cpu));
    memset(&fixture, 0, sizeof(fixture));

    dol_interrupts_init(&fixture.interrupts);
    dol_interrupts_mmio_write(&fixture.interrupts, DOL_PI_INTERRUPT_MASK, 4u,
                              DOL_PI_CAUSE_DSP);
    bluewake_cycle_domain_init(&domain, 1024, NULL, deadline, &fixture);
    bluewake_cycle_domain_begin_turn(&domain, &cpu);
    assert(cpu.cycle_budget == 1);
    assert(cpu.cycle_deadline_active == 1u);
    assert(fixture.deadline_calls == 0u);

    const BluewakeInterruptSources sources = {.dsp = true};
    bluewake_interrupt_sources_publish(&fixture.interrupts, &sources);
    assert(dol_interrupts_external_pending(&fixture.interrupts));
    assert(cpu.cycle_budget == 1);
    assert(cpu.cycle_deadline_active == 1u);
    assert(fixture.deadline_calls == 0u);

    cpu.msr |= PPC_MSR_EE;
    bluewake_interrupt_sources_refresh(&fixture.interrupts, &domain, &cpu,
                                       &sources);
    assert(cpu.cycle_budget == 1);
    assert(cpu.cycle_deadline_active == 1u);
    assert(cpu.cycle_deadline_budget == 1);
    assert(fixture.deadline_calls == 1u);
    return 0;
}
