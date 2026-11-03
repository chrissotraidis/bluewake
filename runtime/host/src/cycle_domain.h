#ifndef BLUEWAKE_CYCLE_DOMAIN_H
#define BLUEWAKE_CYCLE_DOMAIN_H

#include "core/cpu.h"
// See cycle_domain.c: what the host credits each turn, for the credit census.
extern bool g_cycle_credit_census;
extern u64 g_cycle_credit_calls;
extern u64 g_cycle_credit_zero;
extern u64 g_cycle_credit_sum;
extern u64 g_cycle_credit_max;
extern u64 g_cycle_credit_over_budget;

typedef void (*BluewakeCycleAdvanceFn)(CPUState* cpu, u64 cycles, void* user);
typedef u64 (*BluewakeCycleDeadlineFn)(const CPUState* cpu, void* user);

typedef struct BluewakeCycleDomain {
    u64 absolute_cycles;
    u64 dispatch_cycles;
    s64 cap;
    s64 dynamic_near_cap;
    u64 dynamic_threshold;
    BluewakeCycleAdvanceFn advance;
    BluewakeCycleDeadlineFn deadline;
    void* user;
} BluewakeCycleDomain;

void bluewake_cycle_domain_init(BluewakeCycleDomain* domain, s64 cap,
                                BluewakeCycleAdvanceFn advance,
                                BluewakeCycleDeadlineFn deadline, void* user);
void bluewake_cycle_domain_set_dynamic_cap(BluewakeCycleDomain* domain,
                                           s64 near_cap, u64 threshold);
void bluewake_cycle_domain_begin_turn(BluewakeCycleDomain* domain,
                                      CPUState* cpu);
void bluewake_cycle_domain_prepare_dispatch(BluewakeCycleDomain* domain,
                                             CPUState* cpu);
u64 bluewake_cycle_domain_flush(BluewakeCycleDomain* domain, CPUState* cpu);
u64 bluewake_cycle_domain_end_turn(BluewakeCycleDomain* domain,
                                   CPUState* cpu);
u64 bluewake_cycle_domain_observe(BluewakeCycleDomain* domain, CPUState* cpu,
                                  u32 unexecuted_suffix);
void bluewake_cycle_domain_rebudget(BluewakeCycleDomain* domain,
                                    CPUState* cpu);
void bluewake_cycle_domain_write_timebase(BluewakeCycleDomain* domain,
                                          CPUState* cpu, u16 spr, u32 value);

#endif
