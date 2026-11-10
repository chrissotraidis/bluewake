/* cBgW::GroundCrossGrpRp (GZLE01 0x802491F4-0x80249368), natively (#592).
 *
 * The ground check's walk over a collision object's groups: the group filter
 * (a virtual call, dBgW::ChkGrpThrough for stage collision), the x/z and y
 * bounds, the polygon tree (cBgW::GroundCrossRp, run translated) and the
 * children, recursively. In the game's own terms:
 *
 *   if (ChkGrpThrough(grp, chk->passChk, lvl)) return false;
 *   if (outside the group's box in x/z, or below its bottom, or the box's
 *       top below the best hit so far) return false;
 *   hit = tree != 0xFFFF && GroundCrossRp(chk, tree);
 *   for (child = first; child != 0xFFFF; child = next) hit |= walk(child, lvl + 1);
 *   return hit;
 *
 * It plays the same rather than reproducing every guest-visible effect
 * (#542's exact native did that and was 1.03x the translation): it leaves out
 * the walk's own stack frames and saved registers, FPSCR's compare flags and
 * CR0, which nothing reads after the call returns, and charges the walk's
 * cycles as an estimate per group visit instead of block by block. The
 * polygon trees and any filter other than dBgW's run translated, with every
 * cycle they charge. While the walk runs, the turn's budget is held open, so
 * a callee never stops midway: an interrupt that falls due during a walk is
 * serviced right after it (a walk is a few thousand guest cycles).
 *
 * Opt-in: BLUEWAKE_NATIVE_GROUND_WALK=1. Declines (the translation runs)
 * unless direct calls and the edge filter are on, the walk, its filter and
 * the tree test are unwatched by the host, and the stack is plain RAM.
 * No identifier here may be `ctx`. */
#include "native_ground_walk.h"
#include "direct_calls.h"
#include "gather_pipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GW_FILTER 0x800A9684u /* dBgW::ChkGrpThrough */
#define GW_TREE 0x80248C38u   /* cBgW::GroundCrossRp */
#define GW_END 0x80249368u
/* Where a translated callee returns to: outside every chunk's code, so its
 * return dispatch leaves the chunk and bw_call_translated comes back here. */
#define GW_SENTINEL 0x80003100u
#define GW_MAX_DEPTH 24u
/* Cycles charged for the walk's own code: per group visit (prologue,
 * _savegpr_27, the virtual call, the bounds, the table reads, _restgpr_27,
 * epilogue), per child iteration, per filter call (dBgW's, natively here)
 * and per tree call (the call itself; the tree charges its own). From the
 * translation's blocks; #542 measured ~102 cycles a visit with the filter. */
enum { GW_CYC_VISIT = 64, GW_CYC_CHILD = 9, GW_CYC_FILTER = 26, GW_CYC_TREE = 6 };
/* The budget held open while a walk runs. */
#define GW_OPEN ((s64)1 << 40)

/* module_export.c: the chunk that runs a guest address. */
BwChunkFn bw_find_chunk(u32 address);

static int s_enabled = -1;
static int s_unwatched = -1;
static unsigned long long s_walks, s_declined, s_visits, s_trees, s_broken, s_resumed;

static bool gw_enabled(void) {
    if (s_enabled < 0) {
        const char* env = getenv("BLUEWAKE_NATIVE_GROUND_WALK");
        s_enabled = env != NULL && env[0] == '1';
        if (s_enabled)
            fprintf(stderr, "[native-ground-walk] on (plays the same: estimated cycles, no guest stack frames)\n");
    }
    return s_enabled == 1;
}

static bool gw_unwatched(void) {
    if (!bw_edge_watch_ready)
        return false;
    if (s_unwatched < 0) {
        bool clear = bw_edge_unwatched(GW_FILTER) && bw_edge_unwatched(GW_TREE);
        for (u32 a = BLUEWAKE_GROUND_WALK_ENTRY; clear && a < GW_END; a += 4u)
            clear = bw_edge_unwatched(a);
        s_unwatched = clear ? 1 : 0;
    }
    return s_unwatched == 1;
}

static inline u32 gw_word(CPUState* cpu, u32 a) { return bw_mem_read32(cpu, a); }
static inline u32 gw_half(CPUState* cpu, u32 a) { return bw_mem_read16(cpu, a); }
static inline float gw_float(CPUState* cpu, u32 a) {
    const u32 bits = bw_mem_read32(cpu, a);
    float f;
    memcpy(&f, &bits, sizeof f);
    return f;
}

typedef struct {
    CPUState* cpu;
    u32 self, check, filter_target, pass, bounds_base;
    u64 charge;
    bool broken;
} GwWalk;

/* A translated function, called with LR at the sentinel; true when it
 * returned there. A chunk can hand back before that without an exception (a
 * call it does not run directly): the rest runs here, chunk by chunk, as the
 * dispatcher would run it, through addresses the host does not watch. An
 * exception is left alone (the walk is then broken: it is declined up front
 * where one can come, as with floating point off). */
static bool gw_call(GwWalk* w, u32 target) {
    CPUState* cpu = w->cpu;
    cpu->lr = GW_SENTINEL;
    if (!bw_call_translated(cpu, target)) {
        w->broken = true;
        return false;
    }
    for (unsigned hops = 0; cpu->pc != GW_SENTINEL && cpu->exception == 0u && hops < 4096u; ++hops) {
        const u32 pc = cpu->pc;
        if (!bw_edge_unwatched(pc))
            break;
        const BwChunkFn fn = bw_find_chunk(pc);
        if (fn == NULL)
            break;
        s_resumed++;
        ++bw_direct_depth;
        fn(cpu);
        --bw_direct_depth;
    }
    if (cpu->pc != GW_SENTINEL || cpu->exception != 0u) {
        w->broken = true;
        return false;
    }
    return true;
}

/* dBgW::ChkGrpThrough(grp, passChk, lvl): nonzero skips the group. */
static u32 gw_filter_dbgw(GwWalk* w, u32 table, u32 group, u32 depth) {
    if (depth != 2u || w->pass == 0u)
        return 0u;
    const u32 info = gw_word(w->cpu, table + group * 52u + 48u);
    const u32 flags = gw_word(w->cpu, w->pass + 4u);
    if ((info & 0x00080700u) == 0u && (flags & 1u) != 0u)
        return 0u;
    if ((info & 0x100u) != 0u && (flags & 2u) != 0u)
        return 0u;
    if ((info & 0x200u) != 0u && (flags & 4u) != 0u)
        return 0u;
    if ((info & 0x400u) != 0u && (flags & 8u) != 0u)
        return 0u;
    if ((info & 0x80000u) != 0u && (flags & 0x10u) != 0u)
        return 0u;
    return 1u;
}

static bool gw_walk(GwWalk* w, u32 group, u32 depth) {
    CPUState* cpu = w->cpu;
    s_visits++;
    w->charge += GW_CYC_VISIT + GW_CYC_FILTER;
    const u32 table = gw_word(cpu, gw_word(cpu, w->self + 148u) + 36u);
    u32 through;
    if (w->filter_target == GW_FILTER) {
        through = gw_filter_dbgw(w, table, group, depth);
    } else {
        cpu->gpr[3] = w->self;
        cpu->gpr[4] = group;
        cpu->gpr[5] = w->pass;
        cpu->gpr[6] = depth;
        if (!gw_call(w, w->filter_target))
            return false;
        through = cpu->gpr[3];
    }
    if ((through & 0xFFu) != 0u)
        return false;
    const u32 b = w->bounds_base + (group << 5);
    const float x = gw_float(cpu, w->check + 36u), z = gw_float(cpu, w->check + 44u);
    if (gw_float(cpu, b + 4u) > x || gw_float(cpu, b + 16u) < x || gw_float(cpu, b + 12u) > z ||
        gw_float(cpu, b + 24u) < z)
        return false;
    if (!(gw_float(cpu, b + 8u) < gw_float(cpu, w->check + 40u)) ||
        gw_float(cpu, b + 20u) < gw_float(cpu, w->check + 52u))
        return false;
    bool hit = false;
    const u32 tree = gw_half(cpu, table + group * 52u + 46u);
    if (tree != 0xFFFFu) {
        s_trees++;
        w->charge += GW_CYC_TREE;
        cpu->gpr[3] = w->self;
        cpu->gpr[4] = w->check;
        cpu->gpr[5] = tree;
        if (!gw_call(w, GW_TREE))
            return false;
        hit = (cpu->gpr[3] & 0xFFu) != 0u;
    }
    if (depth + 1u >= GW_MAX_DEPTH) {
        w->broken = true;
        return hit;
    }
    for (u32 child = gw_half(cpu, table + group * 52u + 40u); child != 0xFFFFu;
         child = gw_half(cpu, table + child * 52u + 38u)) {
        w->charge += GW_CYC_CHILD;
        if (gw_walk(w, child, depth + 1u))
            hit = true;
        if (w->broken)
            return hit;
    }
    return hit;
}

int bluewake_native_ground_walk_try(CPUState* cpu) {
    if (!gw_enabled())
        return 0;
    const u32 sp = cpu->gpr[1];
    /* Floating point off: the polygon test's first float instruction would
     * trap to the OS's handler midway, so the translation runs instead. */
    if (!bw_direct_enabled || bw_direct_depth + 4u >= BW_DIRECT_DEPTH_MAX || cpu->exception != 0u ||
        (cpu->msr & PPC_MSR_FP) == 0u ||
        !gw_unwatched() || !BW_RAM_FAST_STORE(cpu, sp - 64u, 64u)) {
        s_declined++;
        return 0;
    }
    GwWalk w = {.cpu = cpu, .self = cpu->gpr[3], .check = cpu->gpr[4]};
    const u32 group = cpu->gpr[5], depth = cpu->gpr[6];
    const u32 lr = cpu->lr, vtable = gw_word(cpu, w.self + 4u);
    w.filter_target = gw_word(cpu, vtable + 32u) & ~3u;
    w.pass = gw_word(cpu, w.check + 4u);
    w.bounds_base = gw_word(cpu, w.self + 156u);
    /* A frame for the translated callees: their saved LR lands in it, not in
     * the caller's frame. */
    bw_mem_write32(cpu, sp - 64u, sp);
    cpu->gpr[1] = sp - 64u;
    const s64 downcount = cpu->downcount;
    cpu->downcount = GW_OPEN;
    const bool hit = gw_walk(&w, group, depth);
    const s64 callees = GW_OPEN - cpu->downcount;
    cpu->downcount = downcount - callees - (s64)w.charge;
    cpu->gpr[1] = sp;
    cpu->lr = lr;
    cpu->gpr[3] = hit ? 1u : 0u;
    cpu->cycle_observation_suffix = 0u;
    s_walks++;
    if (w.broken && s_broken++ < 8)
        fprintf(stderr, "[native-ground-walk] a callee did not return to the walk (pc %08X, exception %u)\n",
                cpu->pc, cpu->exception);
    if ((s_walks & ((1u << 16) - 1u)) == 0u)
        fprintf(stderr, "[native-ground-walk] walks=%llu visits=%llu trees=%llu declined=%llu resumed=%llu broken=%llu\n",
                s_walks, s_visits, s_trees, s_declined, s_resumed, s_broken);
    return 1;
}
