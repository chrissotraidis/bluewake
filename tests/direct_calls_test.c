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

/* The gate (#503 M5) with the host's storage: the two flags are the word's
 * first bytes, as in main.c, and gate_host_refresh sets the other two as
 * host_skip_fast_refresh and host_direct_gate_pi do. */
static BwHostGate hg;
static u32 hg_cause, hg_mask;
static bool in_main(u32 address) {
    return address >= BW_GATE_MAIN_FIRST && address < BW_GATE_MAIN_END;
}
static void gate_host_refresh(const BwHostSkipFast* fast) {
    hg.bits.closed = !fast->simple || in_main(fast->single[0]) || in_main(fast->single[1]);
    hg.bits.pi_pending = (hg_cause & hg_mask) != 0u;
}

static void test_gate(void) {
    CPUState cpu = {0};
    cpu.cycle_budget = 20;
    BwHostSkipFast fast = {1u, {0xC1000000u, 0u}, {BW_SKIP_RANGE0_FIRST, BW_SKIP_RANGE1_FIRST},
                           {BW_SKIP_RANGE0_LAST - BW_SKIP_RANGE0_FIRST,
                            BW_SKIP_RANGE1_LAST - BW_SKIP_RANGE1_FIRST}};
    allow = true;
    hg.bits.sources_dirty = hg.bits.decrementer_pending = false;
    assert(bluewake_composite_direct_calls_v3(true, &hg.bits.sources_dirty, &hg.bits.decrementer_pending,
                                             &hg_cause, &hg_mask, can_skip, &allow, &fast, false));
    assert(*bw_direct_gate != 0u); /* closed until the host hands the word over */
    gate_host_refresh(&fast);
    assert(hg.word == 0u);
    assert(bluewake_composite_direct_gate(&hg.word) == 1 && bw_direct_gate == &hg.word);
    unsigned before = queries;
    assert(bw_direct_call_ready_main(&cpu, 0x80009000u) && queries == before); /* no call, no query */

    /* Every host and CPU state, simple or not, a single in the main code or
     * not: the gate's answer is the full check's. */
    static const u32 targets[] = {BW_GATE_MAIN_FIRST, 0x80009000u, BW_SKIP_RANGE0_FIRST - 4u,
                                  BW_GATE_MAIN_END - 4u};
    for (unsigned state = 0; state < 1024u; ++state) {
        hg.bits.sources_dirty = state & 1u;
        hg.bits.decrementer_pending = (state >> 1) & 1u;
        hg_cause = (state >> 2) & 1u;
        hg_mask = (state >> 3) & 1u;
        cpu.msr = (state >> 4) & 1u ? PPC_MSR_EE : 0u;
        cpu.exception = (state >> 5) & 1u;
        cpu.downcount = (state >> 6) & 1u ? -20 : 0;
        bw_direct_depth = (state >> 7) & 1u ? BW_DIRECT_DEPTH_MAX : 0u;
        fast.simple = (state >> 8) & 1u ? 0u : 1u;
        fast.single[1] = (state >> 9) & 1u ? 0x80009000u : 0u;
        gate_host_refresh(&fast);
        for (unsigned i = 0; i < sizeof targets / sizeof targets[0]; ++i)
            assert(bw_direct_call_ready_main(&cpu, targets[i]) ==
                   bw_direct_call_ready_full(&cpu, targets[i]));
    }
    hg.bits.sources_dirty = hg.bits.decrementer_pending = false;
    hg_cause = hg_mask = 0u;
    cpu.msr = 0u;
    cpu.exception = 0u;
    cpu.downcount = 0;
    bw_direct_depth = 0u;
    fast.simple = 1u;
    fast.single[1] = 0u;
    gate_host_refresh(&fast);

    /* Verify keeps the gate closed for the calls; the full check counts what it
     * would have answered. A stale pi_pending byte is an unsafe answer... */
    assert(bluewake_composite_direct_calls_v3(true, &hg.bits.sources_dirty, &hg.bits.decrementer_pending,
                                             &hg_cause, &hg_mask, can_skip, &allow, &fast, true));
    assert(*bw_direct_gate != 0u && hg.word == 0u);
    hg_cause = hg_mask = 1u; /* pending, but the byte was not refreshed */
    cpu.msr = PPC_MSR_EE;
    assert(!bw_direct_call_ready_full(&cpu, 0x80009000u));
    unsigned long long unsafe = 0u, conservative = 0u, answers = 0u;
    bluewake_composite_direct_gate_counts(&unsafe, &conservative, &answers);
    assert(unsafe == 1u && conservative == 0u && answers == 1u);
    /* ...and an interrupt pending while MSR[EE] is clear the conservative side. */
    gate_host_refresh(&fast);
    cpu.msr = 0u;
    assert(bw_direct_call_ready_full(&cpu, 0x80009000u));
    bluewake_composite_direct_gate_counts(&unsafe, &conservative, &answers);
    assert(unsafe == 1u && conservative == 1u && answers == 2u);
    /* Addresses the gate never answers are not counted. */
    assert(bw_direct_call_ready_full(&cpu, 0xC0400000u));
    bluewake_composite_direct_gate_counts(&unsafe, &conservative, &answers);
    assert(answers == 2u);
    hg_cause = hg_mask = 0u;
    gate_host_refresh(&fast);

    /* Off with verify, with the word taken back, and with direct calls off. */
    assert(bluewake_composite_direct_calls_v3(true, &hg.bits.sources_dirty, &hg.bits.decrementer_pending,
                                             &hg_cause, &hg_mask, can_skip, &allow, &fast, false));
    assert(bw_direct_gate == &hg.word);
    assert(bluewake_composite_direct_gate(NULL) == 0 && *bw_direct_gate != 0u);
    assert(bluewake_composite_direct_gate(&hg.word) == 1);
    enable(); /* v2 drops the address-only answer, and with it the gate */
    assert(*bw_direct_gate != 0u);
    assert(!bluewake_composite_direct_calls_v2(false, NULL, NULL, NULL, NULL, NULL, NULL));
    assert(*bw_direct_gate != 0u);
    assert(bluewake_composite_direct_gate(&hg.word) == 0); /* direct calls off: stays closed */
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
    /* The ranges are always direct_skip.h's (main.c checks the host's agree). */
    BwHostSkipFast fast = {1u, {0x80004000u, 0u}, {BW_SKIP_RANGE0_FIRST, BW_SKIP_RANGE1_FIRST},
                           {BW_SKIP_RANGE0_LAST - BW_SKIP_RANGE0_FIRST,
                            BW_SKIP_RANGE1_LAST - BW_SKIP_RANGE1_FIRST}};
    assert(bluewake_composite_direct_calls_v3(true, &attention, &decrementer, &cause, &mask,
                                             can_skip, &allow, &fast, false));
    before = queries;
    assert(!bw_direct_call_ready(&cpu, 0x80004000u)); /* a listed address */
    assert(!bw_direct_call_ready(&cpu, BW_SKIP_RANGE0_FIRST)); /* a range's first address */
    assert(!bw_direct_call_ready(&cpu, BW_SKIP_RANGE0_LAST)); /* and its last */
    assert(!bw_direct_call_ready(&cpu, BW_SKIP_RANGE1_FIRST + 0x10u));
    assert(bw_direct_call_ready(&cpu, BW_SKIP_RANGE0_FIRST - 4u));
    assert(bw_direct_call_ready(&cpu, BW_SKIP_RANGE0_LAST + 4u));
    /* A constant address outside both ranges takes the plain check: the same
     * answer as the full one in every host and CPU state. */
    {
        static const u32 outside[] = {0x80004000u, 0x80009000u, BW_SKIP_RANGE0_FIRST - 4u,
                                      BW_SKIP_RANGE1_LAST + 4u, 0xC0400000u, 0xC13B1850u};
        const unsigned saved_queries = queries;
        for (unsigned state = 0; state < 256u; ++state) {
            attention = state & 1u;
            decrementer = (state >> 1) & 1u;
            cause = (state >> 2) & 1u;
            mask = (state >> 3) & 1u;
            cpu.msr = (state >> 4) & 1u ? PPC_MSR_EE : 0u;
            cpu.exception = (state >> 5) & 1u;
            cpu.downcount = (state >> 6) & 1u ? -20 : 0;
            bw_direct_depth = (state >> 7) & 1u ? BW_DIRECT_DEPTH_MAX : 0u;
            for (unsigned i = 0; i < sizeof outside / sizeof outside[0]; ++i)
                assert(bw_direct_call_ready_plain(&cpu, outside[i]) ==
                       bw_direct_call_ready_full(&cpu, outside[i]));
        }
        assert(queries == saved_queries); /* neither asked the host */
        attention = decrementer = false;
        cause = mask = 0u;
        cpu.msr = 0u;
        cpu.exception = 0u;
        cpu.downcount = 0;
        bw_direct_depth = 0u;
    }
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
    test_gate();
    puts("Direct calls: host contract, inline answer, plain check, gate, interrupts, budget, watches, "
         "mirrors and depth pass");
    return 0;
}
