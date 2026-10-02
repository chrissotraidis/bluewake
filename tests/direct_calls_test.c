#ifdef NDEBUG
#undef NDEBUG
#endif
#include "direct_calls.h"
#include <assert.h>
#include <stdio.h>

static bool attention, decrementer, allow = true;
static u32 cause, mask;
static unsigned queries, dispatched;
static u32 query_address, run_address;
static bool can_skip(void* user, const CPUState* cpu, u32 address) {
    assert(user == &allow && cpu != NULL);
    queries++;
    query_address = address;
    return allow;
}
static void chunk(CPUState* cpu) {
    assert(bw_direct_depth == 1);
    dispatched++;
    run_address = cpu->pc;
    cpu->pc = cpu->lr;
}
BwChunkFn bw_find_chunk(u32 address) {
    return address == 0x80004000u || address == 0xC0400000u ? chunk : NULL;
}
static void enable(void) {
    assert(bluewake_composite_direct_calls_v2(true, &attention, &decrementer,
                                             &cause, &mask, can_skip, &allow));
    assert(bluewake_composite_edge_filter(true));
}

int main(void) {
    CPUState cpu = {0};
    cpu.cycle_budget = 20;
    cpu.lr = 0x80005000u;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u));
    assert(!bluewake_composite_edge_filter(true));
    /* The old donor handshake lacks the changing host observation/input state. */
    assert(!bluewake_composite_direct_calls(true, &attention, &decrementer, &cause, &mask));
    assert(!bluewake_composite_direct_calls_v2(true, &attention, &decrementer,
                                              &cause, &mask, NULL, NULL));
    enable();
    assert(bw_direct_call_ready(&cpu, 0x80004000u));
    assert(query_address == 0x80004000u);
    allow = false; /* Host observes a scene transition or armed jump. */
    assert(!bw_direct_call_ready(&cpu, 0x80004000u));
    allow = true;
    attention = true;
    unsigned before = queries;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u) && queries == before);
    attention = false;
    cpu.msr = PPC_MSR_EE;
    decrementer = true;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u));
    decrementer = false;
    cause = mask = 1;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u));
    cpu.msr = 0;
    assert(bw_direct_call_ready(&cpu, 0x80004000u));
    cpu.exception = 1;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u));
    cpu.exception = 0;
    cpu.downcount = -20;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u));
    cpu.downcount = -19;
    assert(bw_direct_call_ready(&cpu, 0x80004000u));
    bw_direct_depth = BW_DIRECT_DEPTH_MAX;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u));
    bw_direct_depth = 0;
    assert(!bw_direct_call_ready(NULL, 0x80004000u));
    assert(!bw_edge_unwatched(0x80004100u));
    assert(!bw_edge_unwatched(0xC0004100u));
    assert(!bw_edge_unwatched(0xC0400100u));
    assert(!bw_edge_unwatched(0x1000u));
    assert(!bw_call_translated(&cpu, 0x80004100u));
    assert(!bw_call_translated(&cpu, 0x80006000u));
    assert(bw_call_translated(&cpu, 0xC0004000u));
    assert(run_address == 0x80004000u && cpu.pc == cpu.lr && dispatched == 1);
    assert(bw_call_translated(&cpu, 0xC0400000u));
    assert(run_address == 0xC0400000u && dispatched == 2 && bw_direct_depth == 0);
    for (unsigned i = 0; i < BW_EDGE_WATCH_SLOTS; ++i)
        bw_edge_watch_table[i] = 0x80004100u;
    assert(!bw_edge_unwatched(0x80004000u)); /* Corrupt/full table cannot hang. */
    assert(!bluewake_composite_direct_calls_v2(false, NULL, NULL, NULL, NULL, NULL, NULL));
    assert(!bw_edge_filter_enabled && !bw_direct_call_ready(&cpu, 0x80004000u));
    puts("Direct calls: host contract, interrupts, budget, watches, mirrors and depth pass");
    return 0;
}
