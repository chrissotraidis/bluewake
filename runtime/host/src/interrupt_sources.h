#ifndef BLUEWAKE_INTERRUPT_SOURCES_H
#define BLUEWAKE_INTERRUPT_SOURCES_H

#include "cycle_domain.h"
#include "gxruntime/interrupts.h"

typedef struct BluewakeInterruptSources {
    bool di;
    bool dsp;
    bool si;
} BluewakeInterruptSources;

void bluewake_interrupt_sources_publish(
    DolInterrupts* interrupts, const BluewakeInterruptSources* sources);
void bluewake_interrupt_sources_refresh(
    DolInterrupts* interrupts, BluewakeCycleDomain* domain, CPUState* cpu,
    const BluewakeInterruptSources* sources);

#endif
