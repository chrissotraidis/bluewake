#ifdef NDEBUG
#undef NDEBUG
#endif
#define BLUEWAKE_LOOP_CENSUS 1
#include "dispatch_loop.h"

#include <assert.h>
#include <string.h>

/* The census (loop_census.h) counts what goes round the chassis loop without
 * changing the path: the same runs as composite_dispatch_loop_test, counted. */

static unsigned dispatches, services, yield_at;

static int dispatch(CPUState* cpu, u32 address) {
    dispatches++;
    if (address == 0x80009000u)
        return 0; /* no translation */
    cpu->pc = address + 4u;
    cpu->downcount -= 2;
    return 1;
}

static bool edge_service(void* user, CPUState* cpu, u32 address) {
    (void)user; (void)cpu; (void)address;
    return ++services == yield_at;
}

static unsigned long long count_of(unsigned address) {
    for (unsigned i = 0; i < BW_LOOP_CENSUS_SLOTS; ++i)
        if (bw_loop_count[i] != 0u && bw_loop_address[i] == address)
            return bw_loop_count[i];
    return 0u;
}

static CPUState cpu_with_budget(s64 budget) {
    CPUState cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.cycle_budget = budget;
    return cpu;
}

int main(void) {
    /* The turn's budget: one entry, three boundaries, a stop. */
    CPUState cpu = cpu_with_budget(8);
    assert(bluewake_chassis_dispatch_loop(&cpu, 0x80001000u, dispatch, edge_service, NULL));
    assert(dispatches == 4u && services == 3u);
    assert(bw_loop_entries == 1u && bw_loop_boundaries == 3u && bw_loop_skipped == 0u);
    assert(bw_loop_exits[BW_LOOP_EXIT_STOP] == 1u);
    assert(count_of(0x80001004u) == 1u && count_of(0x8000100Cu) == 1u && count_of(0x80001010u) == 0u);

    /* The edge service asks for the host at its second call. */
    cpu = cpu_with_budget(8);
    dispatches = services = 0u;
    yield_at = 2u;
    assert(bluewake_chassis_dispatch_loop(&cpu, 0x80001000u, dispatch, edge_service, NULL));
    assert(dispatches == 2u);
    assert(bw_loop_entries == 2u && bw_loop_boundaries == 5u);
    assert(bw_loop_exits[BW_LOOP_EXIT_SERVICE] == 1u && count_of(0x80001004u) == 2u);

    /* A successor with no translation, and a first dispatch that finds nothing. */
    cpu = cpu_with_budget(8);
    yield_at = 0u;
    assert(bluewake_chassis_dispatch_loop(&cpu, 0x80008FFCu, dispatch, edge_service, NULL));
    assert(bw_loop_exits[BW_LOOP_EXIT_MISS] == 1u && count_of(0x80009000u) == 1u);
    cpu = cpu_with_budget(8);
    assert(!bluewake_chassis_dispatch_loop(&cpu, 0x80009000u, dispatch, edge_service, NULL));
    assert(bw_loop_exits[BW_LOOP_EXIT_FIRST_MISS] == 1u && bw_loop_entries == 4u);

    /* No edge service: the first block only. */
    cpu = cpu_with_budget(8);
    assert(bluewake_chassis_dispatch_loop(&cpu, 0x80001000u, dispatch, NULL, NULL));
    assert(bw_loop_exits[BW_LOOP_EXIT_FIRST_ONLY] == 1u);
    puts("Loop census: entries, boundaries, targets and every exit reason counted");
    return 0;
}
