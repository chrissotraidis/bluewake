#include "interrupt_sources.h"

void bluewake_interrupt_sources_publish(
    DolInterrupts* interrupts, const BluewakeInterruptSources* sources) {
    if (interrupts == NULL || sources == NULL)
        return;

    dol_interrupts_set_source(interrupts, DOL_PI_CAUSE_DI, sources->di);
    dol_interrupts_set_source(interrupts, DOL_PI_CAUSE_DSP, sources->dsp);
    dol_interrupts_set_source(interrupts, DOL_PI_CAUSE_SI, sources->si);
}

void bluewake_interrupt_sources_refresh(
    DolInterrupts* interrupts, BluewakeCycleDomain* domain, CPUState* cpu,
    const BluewakeInterruptSources* sources) {
    if (interrupts == NULL || domain == NULL || cpu == NULL || sources == NULL)
        return;

    bluewake_interrupt_sources_publish(interrupts, sources);
    bluewake_cycle_domain_rebudget(domain, cpu);
}
