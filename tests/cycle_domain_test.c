#ifdef NDEBUG
#undef NDEBUG
#endif
#include "cycle_domain.h"

#include <assert.h>
#include <string.h>

typedef struct Fixture {
    u64 advanced;
    u64 deadline;
    unsigned advances;
    unsigned deadline_calls;
} Fixture;

static BluewakeCycleDomain* observation_domain;

static u32 observe_timebase(CPUState* cpu, u16 spr, u32 cia) {
    (void)cia;
    (void)bluewake_cycle_domain_observe(
        observation_domain, cpu, cpu->cycle_observation_suffix);
    return spr == 269u ? (u32)(cpu->timebase >> 32) : (u32)cpu->timebase;
}

static void advance(CPUState* cpu, u64 cycles, void* user) {
    Fixture* fixture = user;
    fixture->advanced += cycles;
    fixture->advances++;
    cpu->timebase += cycles / 12u;
}

static u64 deadline(const CPUState* cpu, void* user) {
    (void)cpu;
    Fixture* fixture = user;
    fixture->deadline_calls++;
    return fixture->deadline;
}

int main(void) {
    CPUState cpu;
    Fixture fixture = {.deadline = 900u};
    BluewakeCycleDomain domain;
    memset(&cpu, 0, sizeof(cpu));

    bluewake_cycle_domain_init(&domain, 1024, advance, deadline, &fixture);
    bluewake_cycle_domain_begin_turn(&domain, &cpu);
    assert(cpu.cycle_budget == 1);
    assert(cpu.cycle_deadline_active == 1u);
    assert(cpu.cycle_deadline_budget == 1);
    assert(fixture.deadline_calls == 0u);

    bluewake_cycle_domain_prepare_dispatch(&domain, &cpu);
    assert(cpu.cycle_budget == 900);
    assert(cpu.cycle_deadline_active == 1u);
    assert(cpu.cycle_deadline_budget == 900);
    assert(fixture.deadline_calls == 1u);

    cpu.downcount = -144;
    assert(bluewake_cycle_domain_flush(&domain, &cpu) == 144u);
    assert(cpu.downcount == 0);
    assert(domain.absolute_cycles == 144u);
    assert(domain.dispatch_cycles == 144u);
    assert(fixture.advanced == 144u);
    assert(fixture.advances == 1u);
    assert(cpu.timebase == 12u);

    cpu.downcount = -30;
    assert(bluewake_cycle_domain_observe(&domain, &cpu, 11u) == 19u);
    assert(cpu.downcount == -11);
    assert(domain.absolute_cycles == 163u);
    assert(domain.dispatch_cycles == 163u);
    assert(fixture.advanced == 163u);
    assert(bluewake_cycle_domain_flush(&domain, &cpu) == 11u);
    assert(domain.absolute_cycles == 174u);

    assert(fixture.advances == 3u); // flush, observe, remaining suffix
    assert(bluewake_cycle_domain_flush(&domain, &cpu) == 0u);
    assert(fixture.advances == 3u); // an empty flush does not advance devices

    fixture.deadline = 0u;
    bluewake_cycle_domain_rebudget(&domain, &cpu);
    assert(cpu.cycle_budget == 1);
    assert(cpu.cycle_deadline_budget == 1);

    fixture.deadline = 4096u;
    bluewake_cycle_domain_rebudget(&domain, &cpu);
    assert(cpu.cycle_budget == 1024);
    assert(cpu.cycle_deadline_active == 0u);
    assert(cpu.cycle_deadline_budget == 4096);

    bluewake_cycle_domain_set_dynamic_cap(&domain, 256, 1024u);
    fixture.deadline = 900u;
    bluewake_cycle_domain_rebudget(&domain, &cpu);
    assert(cpu.cycle_budget == 256);
    assert(cpu.cycle_deadline_budget == 900);
    fixture.deadline = 200u;
    bluewake_cycle_domain_rebudget(&domain, &cpu);
    assert(cpu.cycle_budget == 200);
    assert(cpu.cycle_deadline_budget == 200);
    fixture.deadline = 4096u;
    bluewake_cycle_domain_rebudget(&domain, &cpu);
    assert(cpu.cycle_budget == 1024);
    assert(cpu.cycle_deadline_budget == 4096);
    bluewake_cycle_domain_set_dynamic_cap(&domain, 0, 0u);

    bluewake_cycle_domain_begin_turn(&domain, &cpu);
    assert(domain.dispatch_cycles == 0u);
    assert(domain.absolute_cycles == 174u);
    assert(cpu.cycle_budget == 1);
    assert(cpu.cycle_deadline_active == 1u);
    assert(cpu.cycle_deadline_budget == 1);

    cpu.downcount = -12;
    bluewake_cycle_domain_prepare_dispatch(&domain, &cpu);
    assert(domain.dispatch_cycles == 12u);
    assert(domain.absolute_cycles == 186u);
    assert(cpu.downcount == 0);

    observation_domain = &domain;
    cpu.spr_read = observe_timebase;
    cpu.downcount = -24;
    cpu.cycle_observation_suffix = 6u;
    assert(ppc_mftb(&cpu, 268u, 0x80001000u) == 15u);
    assert(cpu.downcount == -6);
    assert(domain.absolute_cycles == 204u);
    assert(bluewake_cycle_domain_flush(&domain, &cpu) == 6u);

    const unsigned deadline_calls = fixture.deadline_calls;
    cpu.downcount = -7;
    cpu.cycle_budget = 333;
    assert(bluewake_cycle_domain_end_turn(&domain, &cpu) == 7u);
    assert(fixture.deadline_calls == deadline_calls);
    assert(cpu.cycle_budget == 333);

    memset(&cpu, 0, sizeof(cpu));
    memset(&fixture, 0, sizeof(fixture));
    fixture.deadline = UINT64_MAX;
    bluewake_cycle_domain_init(&domain, 1024, advance, deadline, &fixture);
    cpu.timebase = 0x1122334455667788ull;
    cpu.downcount = -30;
    cpu.cycle_observation_suffix = 6u;
    bluewake_cycle_domain_write_timebase(&domain, &cpu, 284u, 0xAABBCCDDu);
    assert(domain.absolute_cycles == 24u);
    assert(fixture.advanced == 24u);
    assert(cpu.downcount == -6);
    assert(cpu.timebase == 0x11223344AABBCCDDull);
    return 0;
}
