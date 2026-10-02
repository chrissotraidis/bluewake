/* The state behind direct calls between chunks (direct_calls.h). */
#include "direct_calls.h"

#if defined(_WIN32)
#define BW_DIRECT_EXPORT __declspec(dllexport)
#else
#define BW_DIRECT_EXPORT __attribute__((visibility("default")))
#endif

/* Off until the host hands over its flags; these keep the ready test safe to
 * read before then. */
static const bool k_attention = true;
static const bool k_clear = false;
static const u32 k_zero = 0u;

unsigned bw_direct_depth;
bool bw_direct_enabled;
const bool* bw_host_sources_dirty = &k_attention;
const bool* bw_host_decrementer_pending = &k_clear;
const u32* bw_host_pi_cause = &k_zero;
const u32* bw_host_pi_mask = &k_zero;
BwHostCanSkipFn bw_host_can_skip;
void* bw_host_can_skip_user;

u32 bw_edge_watch_table[BW_EDGE_WATCH_SLOTS];
bool bw_edge_watch_ready;
bool bw_edge_filter_enabled;

#ifdef BLUEWAKE_EDGE_FILTER
#ifndef BW_EDGE_WATCH_INCLUDE
#define BW_EDGE_WATCH_INCLUDE "bw_edge_watch.inc"
#endif
#include BW_EDGE_WATCH_INCLUDE /* static const u32 bw_edge_watch_list[] */
_Static_assert(sizeof bw_edge_watch_list / sizeof bw_edge_watch_list[0] < BW_EDGE_WATCH_SLOTS,
               "direct-call watch list must leave an empty hash-table slot");

static void edge_watch_build(void) {
    if (bw_edge_watch_ready)
        return;
    for (u32 i = 0; i < sizeof bw_edge_watch_list / sizeof bw_edge_watch_list[0]; ++i) {
        const u32 canonical = bw_edge_watch_list[i] & ~0x40000000u;
        u32 slot = (canonical * 0x9E3779B1u) >> 20;
        while (bw_edge_watch_table[slot] != 0u && bw_edge_watch_table[slot] != canonical)
            slot = (slot + 1u) & (BW_EDGE_WATCH_SLOTS - 1u);
        bw_edge_watch_table[slot] = canonical;
    }
    bw_edge_watch_ready = true;
}
#endif

/* The dispatcher's own lookup (module_export.c): the chunk that runs a guest
 * address, with the mods' variants and the native entries it resolves. */
BwChunkFn bw_find_chunk(u32 address);

bool bw_call_translated(CPUState* cpu, u32 target) {
    if (!bw_edge_watch_ready || !bw_edge_unwatched(target))
        return false;
    /* The main executable's code through its mirror runs at the plain address,
     * as the dispatcher's slow path resolves it. */
    const u32 canonical = target & ~0x40000000u;
    const u32 run = canonical >= 0x80003100u && canonical < 0x80400000u ? canonical : target;
    const BwChunkFn fn = bw_find_chunk(run);
    if (fn == NULL)
        return false;
    cpu->pc = run;
    ++bw_direct_depth;
    fn(cpu);
    --bw_direct_depth;
    return true;
}

/* Skip the host's edge service at boundaries where it would do nothing
 * (dispatch_loop.h), and let indirect calls run directly: both need the
 * builder's watch list and direct calls on. Returns 1 when on. */
BW_DIRECT_EXPORT int bluewake_composite_edge_filter(bool enabled) {
#ifdef BLUEWAKE_EDGE_FILTER
    if (enabled && bw_direct_enabled) {
        edge_watch_build();
        bw_edge_filter_enabled = true;
        return 1;
    }
#endif
    (void)enabled;
    bw_edge_filter_enabled = false;
    return 0;
}

/* The host's edge-service state, read before and after each direct call.
 * enabled false (or NULL flags) turns direct calls off: every call then goes
 * round the chassis loop. Returns 1 when direct calls are on. */
BW_DIRECT_EXPORT int bluewake_composite_direct_calls(bool enabled, const bool* sources_dirty,
                                                     const bool* decrementer_pending, const u32* pi_cause,
                                                     const u32* pi_mask) {
    /* The donor handshake cannot describe BlueWake's extra host observations. */
    (void)enabled; (void)sources_dirty; (void)decrementer_pending;
    (void)pi_cause; (void)pi_mask;
    bw_direct_enabled = false;
    bw_edge_filter_enabled = false;
    bw_host_can_skip = NULL;
    bw_host_can_skip_user = NULL;
    return 0;
}

BW_DIRECT_EXPORT int bluewake_composite_direct_calls_v2(
    bool enabled, const bool* sources_dirty, const bool* decrementer_pending,
    const u32* pi_cause, const u32* pi_mask, BwHostCanSkipFn can_skip, void* user) {
    if (!enabled || sources_dirty == NULL || decrementer_pending == NULL || pi_cause == NULL ||
        pi_mask == NULL || can_skip == NULL) {
        bw_direct_enabled = false;
        bw_edge_filter_enabled = false;
        bw_host_can_skip = NULL;
        bw_host_can_skip_user = NULL;
        return 0;
    }
    bw_host_sources_dirty = sources_dirty;
    bw_host_decrementer_pending = decrementer_pending;
    bw_host_pi_cause = pi_cause;
    bw_host_pi_mask = pi_mask;
    bw_host_can_skip = can_skip;
    bw_host_can_skip_user = user;
    bw_direct_enabled = true;
    return 1;
}
