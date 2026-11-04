#ifndef BLUEWAKE_COMPOSITE_DIRECT_CALLS_H
#define BLUEWAKE_COMPOSITE_DIRECT_CALLS_H

/* Direct calls between translated chunks (scripts/windows/direct_calls.py).
 *
 * A bl into another chunk ends the caller's chunk: it returns to the chassis
 * loop (dispatch_loop.h), which checks the turn, asks the host's edge service
 * about the target and dispatches it; when the callee's blr names the return
 * address, which is back in the caller's chunk, the same happens again. The
 * rewritten call does what the loop would do, without going round it: the
 * loop's own checks, then the callee's chunk through the chunk table (so an
 * enabled mod's variant is the one called), and, when control comes back to
 * the return address, the caller's block there - as the translator already
 * does for a call inside one chunk.
 *
 * The one thing the loop does that the module cannot is the edge service. The
 * service answers "nothing to do" at every address the host does not name
 * whenever its state flags are clear; the transform leaves every call whose
 * target or return address the host names alone, and bw_direct_call_ready
 * reads the host's flags (the interrupt sources, a pending interrupt the guest
 * would take) through pointers the host hands over. Until it does, and in any
 * diagnostic mode, direct calls are off and every call goes round the loop.
 *
 * Anything else - a callee that stops for the cycle budget, an exception, a
 * return somewhere else - leaves the caller's chunk with ctx->pc where the
 * guest is, and the loop continues from there as it always did.
 *
 * No identifier here may be `ctx`: the chunks define it as a macro. */

#include "core/cpu.h"
#include "direct_skip.h"

#define BW_DIRECT_DEPTH_MAX 32u

typedef void (*BwChunkFn)(CPUState*);
extern BwChunkFn* const bw_chunk_fns;       /* module_export.c: the dispatch table */
extern unsigned bw_direct_depth;
extern bool bw_direct_enabled;
extern const bool* bw_host_sources_dirty;
extern const bool* bw_host_decrementer_pending;
extern const u32* bw_host_pi_cause;
extern const u32* bw_host_pi_mask;
/* BlueWake's host has address-independent observations and input hooks beyond
 * the donor interrupt flags. A versioned, read-only query must also approve
 * each skipped boundary. Older hosts leave the optimization disabled. */
typedef bool (*BwHostCanSkipFn)(void*, const CPUState*, u32);
extern BwHostCanSkipFn bw_host_can_skip;
extern void* bw_host_can_skip_user;
int bluewake_composite_direct_calls(bool, const bool*, const bool*, const u32*, const u32*);
int bluewake_composite_direct_calls_v2(bool, const bool*, const bool*, const u32*,
                                     const u32*, BwHostCanSkipFn, void*);
/* v2 plus the host's address-only answer (direct_skip.h) and the verify switch. */
int bluewake_composite_direct_calls_v3(bool, const bool*, const bool*, const u32*,
                                     const u32*, BwHostCanSkipFn, void*,
                                     const BwHostSkipFast*, bool);
extern const volatile BwHostSkipFast* bw_host_skip_fast;
extern bool bw_host_skip_verify;
void bw_host_skip_verify_check(const CPUState* cpu, u32 address, bool allowed);
void bluewake_composite_direct_skip_counts(unsigned long long* unsafe,
                                           unsigned long long* conservative,
                                           unsigned long long* inline_answers);
/* The host's gate word (direct_skip.h), or a closed one: closed until the host
 * hands it over, and whenever direct calls, the address-only answer or verify
 * change it (direct_calls.c). */
extern const volatile u32* bw_direct_gate;
int bluewake_composite_direct_gate(const volatile u32* gate);
/* With verify on, every direct call takes the full check, which also asks what
 * the gate would have answered and counts disagreements. */
void bw_direct_gate_verify(const CPUState* cpu, u32 address, bool exact);
void bluewake_composite_direct_gate_counts(unsigned long long* unsafe,
                                           unsigned long long* conservative,
                                           unsigned long long* answers);
int bluewake_composite_edge_filter(bool);

/* The host's edge service has nothing to do at an address it does not watch:
 * its interrupt sources are clean and no interrupt the guest would take is
 * pending. */
static inline bool bw_host_quiet(const CPUState* cpu) {
    if (*bw_host_sources_dirty)
        return false;
    return (cpu->msr & PPC_MSR_EE) == 0u ||
           (!*bw_host_decrementer_pending && (*bw_host_pi_cause & *bw_host_pi_mask) == 0u);
}

/* The chassis loop's checks before a dispatch (an exception, the turn's
 * budget), and the edge service's before it would have nothing to do. */
/* Both checks stay out of line and with the hot code, as the single check was:
 * inlined at every call site the chunks grow about 11% and run slower (#503,
 * Steam Deck), and the trained profile does not know these names. */
static inline __attribute__((always_inline)) bool bw_direct_call_ready_exact(const CPUState* cpu, u32 address) {
    if (!bw_direct_enabled || cpu == NULL || bw_host_can_skip == NULL ||
        bw_direct_depth >= BW_DIRECT_DEPTH_MAX)
        return false;
    if (cpu->exception != 0u || (cpu->cycle_budget > 0 && cpu->downcount <= -cpu->cycle_budget))
        return false;
    if (!bw_host_quiet(cpu))
        return false;
    /* The host's answer read inline when only the address matters (direct_skip.h),
     * instead of a call into the host for every direct call. */
    const volatile BwHostSkipFast* fast = bw_host_skip_fast;
    if (fast != NULL && fast->simple) {
        const bool allowed = address != fast->single[0] && address != fast->single[1] &&
                             address - fast->range_first[0] > fast->range_span[0] &&
                             address - fast->range_first[1] > fast->range_span[1];
        if (__builtin_expect(bw_host_skip_verify, 0))
            bw_host_skip_verify_check(cpu, address, allowed);
        return allowed;
    }
    return bw_host_can_skip(bw_host_can_skip_user, cpu, address);
}

static __attribute__((noinline, hot, unused)) bool bw_direct_call_ready_full(const CPUState* cpu, u32 address) {
    const bool exact = bw_direct_call_ready_exact(cpu, address);
    if (__builtin_expect(bw_host_skip_verify, 0))
        bw_direct_gate_verify(cpu, address, exact);
    return exact;
}

/* The same answer for an address the build knows lies outside the host's two
 * ranges (direct_skip.h): with the inline answer on, only the singles remain.
 * bw_host_skip_fast is set only while direct calls are on, so it stands for
 * bw_direct_enabled and the host's query; the translated code's cpu is never
 * NULL. Anything else (the answer off, verify) is the full check. */
static __attribute__((noinline, hot, unused)) bool bw_direct_call_ready_plain(const CPUState* cpu, u32 address) {
    const volatile BwHostSkipFast* fast = bw_host_skip_fast;
    if (__builtin_expect(fast != NULL, 1) && fast->simple && !bw_host_skip_verify) {
        if (bw_direct_depth >= BW_DIRECT_DEPTH_MAX || cpu->exception != 0u ||
            (cpu->cycle_budget > 0 && cpu->downcount <= -cpu->cycle_budget) || !bw_host_quiet(cpu))
            return false;
        return address != fast->single[0] && address != fast->single[1];
    }
    return bw_direct_call_ready_full(cpu, address);
}

static inline bool bw_skip_range_outside(u32 address) {
    return (address < BW_SKIP_RANGE0_FIRST || address > BW_SKIP_RANGE0_LAST) &&
           (address < BW_SKIP_RANGE1_FIRST || address > BW_SKIP_RANGE1_LAST);
}

static inline bool bw_skip_main_code(u32 address) {
    return address >= BW_GATE_MAIN_FIRST && address < BW_GATE_MAIN_END;
}

/* What remains of the plain check while the gate is open: the host-side tests
 * all hold (direct_skip.h), so only the depth, an exception and the budget. */
static inline __attribute__((always_inline)) bool bw_direct_gate_ready(const CPUState* cpu) {
    return bw_direct_depth < BW_DIRECT_DEPTH_MAX && cpu->exception == 0u &&
           !(cpu->cycle_budget > 0 && cpu->downcount <= -cpu->cycle_budget);
}

/* A target in the main executable's code, outside both ranges: with the gate
 * open it stands for every host-side test, read inline without a call (#503
 * M5); closed, the plain check answers as before. */
static inline __attribute__((always_inline)) bool bw_direct_call_ready_main(const CPUState* cpu, u32 address) {
    if (__builtin_expect(*bw_direct_gate == 0u, 1))
        return bw_direct_gate_ready(cpu);
    return bw_direct_call_ready_plain(cpu, address);
}

/* At a call the translator wrote, the address is a constant: the range tests
 * are settled here, at build time, and so is whether the target is in the main
 * executable's code. */
static inline __attribute__((always_inline)) bool bw_direct_call_ready(const CPUState* cpu, u32 address) {
    if (__builtin_constant_p(address) && bw_skip_range_outside(address)) {
        if (bw_skip_main_code(address))
            return bw_direct_call_ready_main(cpu, address);
        return bw_direct_call_ready_plain(cpu, address);
    }
    return bw_direct_call_ready_full(cpu, address);
}

/* The addresses the host's edge service acts at, from the builder's watch list
 * (scripts/windows/direct_calls.py writes bw_edge_watch.inc: every guest
 * address the host sources name), as an open-addressed set of canonical
 * addresses (the 0x40000000 mirror bit clear, as the service tests them). A
 * boundary anywhere else in translated code - the main executable's, in
 * either mirror, or a REL module's linked code - is one the service lets
 * pass; one outside it (a module's raw image, say) it may not. */
#define BW_EDGE_WATCH_SLOTS 4096u
extern u32 bw_edge_watch_table[BW_EDGE_WATCH_SLOTS];
extern bool bw_edge_watch_ready;
extern bool bw_edge_filter_enabled;

static inline bool bw_edge_unwatched(u32 address) {
    const u32 canonical = address & ~0x40000000u;
    const bool main_code = canonical >= 0x80003100u && canonical < 0x80400000u;
    const bool rel_code = address >= 0xC0400000u && address < 0xC2000000u;
    if (!(main_code || rel_code))
        return false;
    u32 slot = (canonical * 0x9E3779B1u) >> 20;
    for (u32 probes = 0; probes < BW_EDGE_WATCH_SLOTS; ++probes) {
        const u32 entry = bw_edge_watch_table[slot];
        if (entry == 0u)
            return true;
        if (entry == canonical)
            return false;
        slot = (slot + 1u) & (BW_EDGE_WATCH_SLOTS - 1u);
    }
    return false;
}

/* An indirect call's target run the way the dispatcher would run it, when the
 * host would have nothing to do at it: false sends the call round the loop.
 * The caller has checked bw_direct_call_ready. */
bool bw_call_translated(CPUState* cpu, u32 target);

/* A direct call to a leaf with a native form (native_math.c's matrix leaves,
 * native_vec.c's vector leaves): the native, where native math is on, as a
 * dispatch of a matrix leaf runs it first; zero, with nothing changed, sends
 * the call to the translated body. */
int bw_native_call(CPUState* cpu, u32 address);

#endif
