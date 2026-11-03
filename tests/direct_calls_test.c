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

    /* v3: the host's address-only answer, read inline without a call. */
    BwHostSkipFast fast = {1u, {0x80004000u, 0u}, {0x80008000u, 0xFFFFFFFFu}, {0x100u, 0u}};
    assert(bluewake_composite_direct_calls_v3(true, &attention, &decrementer, &cause, &mask,
                                             can_skip, &allow, &fast, false));
    before = queries;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u)); /* a listed address */
    assert(!bw_direct_call_ready(&cpu, 0x80008000u)); /* a range's first address */
    assert(!bw_direct_call_ready(&cpu, 0x80008100u)); /* and its last */
    assert(bw_direct_call_ready(&cpu, 0x80007FFCu));
    assert(bw_direct_call_ready(&cpu, 0x80008104u));
    allow = false; /* The inline answer does not ask the host... */
    assert(bw_direct_call_ready(&cpu, 0x80009000u) && queries == before);
    attention = true; /* ...but the interrupt sources still come first. */
    assert(!bw_direct_call_ready(&cpu, 0x80009000u));
    attention = false;
    fast.simple = 0u; /* Not simple: the host answers, as with v2. */
    assert(!bw_direct_call_ready(&cpu, 0x80009000u) && queries == before + 1u);
    allow = true;
    assert(bw_direct_call_ready(&cpu, 0x80009000u) && queries == before + 2u);
    /* The verify switch asks the host as well and counts disagreements. */
    fast.simple = 1u;
    assert(bluewake_composite_direct_calls_v3(true, &attention, &decrementer, &cause, &mask,
                                             can_skip, &allow, &fast, true));
    allow = false;
    assert(bw_direct_call_ready(&cpu, 0x80009000u)); /* inline yes, host no: unsafe */
    allow = true;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u)); /* inline no, host yes: conservative */
    unsigned long long unsafe = 0u, conservative = 0u, inline_answers = 0u;
    bluewake_composite_direct_skip_counts(&unsafe, &conservative, &inline_answers);
    assert(unsafe == 1u && conservative == 1u && inline_answers == 2u);
    /* v2 drops the inline answer. */
    enable();
    allow = false;
    assert(!bw_direct_call_ready(&cpu, 0x80009000u));
    allow = true;
    /* v3 that fails to enable leaves nothing behind. */
    assert(!bluewake_composite_direct_calls_v3(true, NULL, NULL, NULL, NULL, NULL, NULL, &fast, true));
    assert(bw_host_skip_fast == NULL && !bw_host_skip_verify);

    assert(!bluewake_composite_direct_calls_v2(false, NULL, NULL, NULL, NULL, NULL, NULL));
    assert(!bw_edge_filter_enabled && !bw_direct_call_ready(&cpu, 0x80004000u));
    puts("Direct calls: host contract, inline answer, interrupts, budget, watches, mirrors and depth pass");
    return 0;
}
