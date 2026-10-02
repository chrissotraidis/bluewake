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
static inline bool bw_direct_call_ready(const CPUState* cpu, u32 address) {
    if (!bw_direct_enabled || cpu == NULL || bw_host_can_skip == NULL ||
        bw_direct_depth >= BW_DIRECT_DEPTH_MAX)
        return false;
    if (cpu->exception != 0u || (cpu->cycle_budget > 0 && cpu->downcount <= -cpu->cycle_budget))
        return false;
    return bw_host_quiet(cpu) && bw_host_can_skip(bw_host_can_skip_user, cpu, address);
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
