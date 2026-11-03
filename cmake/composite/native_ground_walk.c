/* cBgW::GroundCrossGrpRp (GZLE01 0x802491F4-0x80249368), natively (#542).
 *
 * The ground check's walk over a collision object's groups: the group filter
 * (dBgW::ChkGrpThrough, a virtual call), the x/z and y bounds, the polygon
 * tree (cBgW::GroundCrossRp) and the children, recursively.
 *
 * This does what the translation's prepaid block copies do, block by block,
 * on the CPU state itself: the same registers, CR0, FPSCR (through
 * ppc_fcmp), stack stores, inline _savegpr_27/_restgpr_27, cycle charges and
 * cycle suffix, in the same order, through the same memory helpers
 * (gather_pipe.h). What it leaves out is the per-instruction machinery: pc
 * stores, the out-of-line tests a plain RAM access never takes, and the call
 * and return dispatch inside the chunk.
 *
 * It stops, with every effect before that point made and pc set, only where
 * the translation itself can hand the dispatcher a pc (a budget check, a
 * helper or call that is not ready), or at a block's label when it cannot go
 * on exactly: a read that is not plain RAM, a loop label that cannot prepay,
 * a polygon tree (GroundCrossRp stays translated). The dispatcher then
 * resumes the translation there. That needs the dispatcher to pass those
 * boundaries silently, which the entry checks: the edge filter on, the host
 * quiet, the walk's labels and its callees unwatched.
 *
 * Opt-in (#542): off unless the host enables it (BLUEWAKE_NATIVE_GROUND_WALK=1).
 * tests/native_bg_test.c --walk compares it with the translation, every CPU
 * byte and MEM1. No identifier here may be `ctx`. */
#include "native_ground_walk.h"
#include "native_bg.h"
#include "direct_calls.h"
#include "inline_gpr.h"
#include "gather_pipe.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#define BW_WALK_EXPORT __declspec(dllexport)
#else
#define BW_WALK_EXPORT __attribute__((visibility("default")))
#endif

enum { GW_DECLINE = 0, GW_RETURN = 1, GW_STOP = 2 };
enum { GW_MAX_DEPTH = 16 };
/* Cycles the native keeps in hand at every budget check: more than the walk
 * runs between two of its checks (≈ 120, with the filter's leaf). So the turn
 * cannot run out where the native must stop without a check (a read that is
 * not plain RAM, a polygon tree): there the dispatcher resumes at once, as the
 * translation would have gone on. Without the headroom it stops at the check,
 * which the dispatcher also resumes at once. */
enum { GW_HEADROOM = 256 };

#define GW_FILTER 0x800A9684u   /* dBgW::ChkGrpThrough, the certified virtual target */
#define GW_TREE 0x80248C38u     /* cBgW::GroundCrossRp */
#define GW_SAVE 0x80328F38u     /* _savegpr_27 */
#define GW_RESTORE 0x80328F84u  /* _restgpr_27 */
#define GW_CHILD_RETURN 0x80249328u

static BluewakeGroundWalkReady s_ready;
static void* s_ready_user;
static int s_unwatched = -1; /* the walk's labels and callees unwatched (computed once) */
static unsigned long long s_returned, s_stopped, s_declined, s_visits;

BW_WALK_EXPORT int bluewake_composite_ground_walk_v1(bool enabled, BluewakeGroundWalkReady ready, void* user) {
    s_ready = enabled ? ready : NULL;
    s_ready_user = s_ready != NULL ? user : NULL;
    s_unwatched = -1;
    return s_ready != NULL;
}

BW_WALK_EXPORT void bluewake_ground_walk_report(void) {
    fprintf(stderr, "[native-ground-walk] returned=%llu stopped=%llu declined=%llu visits=%llu\n", s_returned,
            s_stopped, s_declined, s_visits);
}

/* generated.h's dolrecomp_f32_from_bits: lfs's single to double. */
static inline __attribute__((always_inline)) f64 gw_f32(u32 bits) {
    u64 x = bits;
    u64 exp = (x >> 23) & 0xFFu;
    u64 frac = x & 0x007FFFFFu;
    u64 result;
    if (exp > 0 && exp < 255) {
        u64 y = !(exp >> 7);
        u64 z = (y << 61) | (y << 60) | (y << 59);
        result = ((x & 0xC0000000u) << 32) | z | ((x & 0x3FFFFFFFu) << 29);
    } else if (exp == 0 && frac != 0) {
        exp = 1023 - 126;
        do {
            frac <<= 1;
            exp -= 1;
        } while ((frac & 0x00800000u) == 0);
        result = ((x & 0x80000000u) << 32) | (exp << 52) | ((frac & 0x007FFFFFu) << 29);
    } else {
        u64 y = exp >> 7;
        u64 z = (y << 61) | (y << 60) | (y << 59);
        result = ((x & 0xC0000000u) << 32) | z | ((x & 0x3FFFFFFFu) << 29);
    }
    f64 value;
    memcpy(&value, &result, sizeof value);
    return value;
}

static inline bool gw_over(const CPUState* cpu) { return cpu->downcount <= -cpu->cycle_budget; }
static inline bool gw_short(const CPUState* cpu) { return cpu->downcount - GW_HEADROOM <= -cpu->cycle_budget; }

/* generated.h's dolrecomp_block_can_precharge. */
static inline bool gw_can_precharge(const CPUState* cpu, u32 cycles) {
    if (cpu->cycle_deadline_budget <= 0)
        return true;
    const s64 remaining = cpu->cycle_deadline_budget + cpu->downcount;
    return remaining >= 0 && (u64)remaining >= (u64)cycles;
}
static inline bool gw_ram(CPUState* cpu, u32 address, u32 size) { return BW_RAM_FAST(cpu, address, size); }
static inline bool gw_store(CPUState* cpu, u32 address, u32 size) { return BW_RAM_FAST_STORE(cpu, address, size); }

/* CR0 from a record form (signed against zero) or a cmplwi, with XER's SO. */
static inline void gw_cr0_record(CPUState* cpu, u32 value) {
    const s32 v = (s32)value;
    const u32 bits = (v < 0 ? 0x8u : v > 0 ? 0x4u : 0x2u) | ((cpu->xer >> 31) & 1u);
    cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (bits << 28);
}
static inline void gw_cr0_logical(CPUState* cpu, u32 a, u32 b) {
    const u32 bits = (a < b ? 0x8u : a > b ? 0x4u : 0x2u) | ((cpu->xer >> 31) & 1u);
    cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (bits << 28);
}

/* lfs fN, d(rA): writes the cycle suffix first, as the fast copy does. The
 * address was checked as plain RAM, so the load is bw_mem_read32's fast form
 * (mem_read32 reads the same bytes there). */
static inline __attribute__((always_inline)) void gw_lfs(CPUState* cpu, unsigned reg, u32 address, u32 suffix) {
    cpu->cycle_observation_suffix = suffix;
    const f64 value = gw_f32(bw_mem_read32(cpu, address));
    cpu->fpr[reg] = value;
    cpu->ps1[reg] = value;
}

/* fcmpo cr0: ppc_fcmp's ordered result inline (CR0 and FPSCR's FPCC, OR-ed in
 * as ppc_fcmp does); a NaN takes ppc_fcmp itself (its invalid-operation bits). */
static inline __attribute__((always_inline)) void gw_fcmpo(CPUState* cpu, f64 a, f64 b) {
    if (__builtin_expect(a != a || b != b, 0)) {
        ppc_fcmp(cpu, 0, a, b, true);
        return;
    }
    const u32 compare = a < b ? 0x8u : a > b ? 0x4u : 0x2u;
    cpu->fpscr |= compare << 12;
    cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (compare << 28);
}

/* A precharge label (the child loop's): stop where the translation would stop
 * for the budget, or where it would not prepay (it then charges instruction by
 * instruction, which the dispatcher resumes exactly from the label). */
static inline bool gw_precharge(CPUState* cpu, u32 label, u32 cycles) {
    if (!gw_can_precharge(cpu, cycles) || gw_short(cpu)) {
        cpu->pc = label;
        return false;
    }
    cpu->downcount -= cycles;
    return true;
}

static bool gw_boundaries_unwatched(void) {
    if (s_unwatched < 0) {
        bool clear = bw_edge_unwatched(GW_FILTER) && bw_edge_unwatched(GW_TREE);
        for (u32 a = BLUEWAKE_GROUND_WALK_ENTRY; clear && a < 0x80249368u; a += 4u)
            clear = bw_edge_unwatched(a);
        s_unwatched = clear ? 1 : 0;
    }
    return s_unwatched == 1;
}

/* One call of the walk, entered at 0x802491F4 with LR the return address.
 * GW_RETURN: it returned (pc = LR's target); GW_STOP: pc set for the
 * dispatcher. */
static int gw_walk(CPUState* cpu, unsigned depth) {
    /* 802491F4 (5): stwu r1,-32(r1); mflr r0; stw r0,36(r1); addi r11,r1,32 */
    if (gw_short(cpu) || depth >= GW_MAX_DEPTH || !gw_store(cpu, cpu->gpr[1] - 32u, 4u) ||
        !gw_store(cpu, cpu->gpr[1] - 32u + 36u, 4u)) {
        cpu->pc = 0x802491F4u;
        return GW_STOP;
    }
    s_visits++;
    cpu->downcount -= 5;
    {
        const u32 ea = cpu->gpr[1] - 32u;
        bw_mem_write32(cpu, ea, cpu->gpr[1]);
        cpu->gpr[1] = ea;
    }
    cpu->gpr[0] = cpu->lr;
    bw_mem_write32(cpu, cpu->gpr[1] + 36u, cpu->gpr[0]);
    cpu->gpr[11] = cpu->gpr[1] + 32u;
    /* 80249204: bl _savegpr_27, inline */
    cpu->lr = 0x80249208u;
    cpu->pc = GW_SAVE;
    if (!bw_direct_call_ready(cpu, GW_SAVE) || !bw_inline_gpr_memory_ready(cpu, 27u) ||
        !(cpu->downcount - 5 > -cpu->cycle_budget))
        return GW_STOP;
    cpu->downcount -= 6;
    {
        u8* const slots = bw_inline_gpr_frame(cpu, 27u, true);
        write_be32(slots + 0, cpu->gpr[27]);
        write_be32(slots + 4, cpu->gpr[28]);
        write_be32(slots + 8, cpu->gpr[29]);
        write_be32(slots + 12, cpu->gpr[30]);
        write_be32(slots + 16, cpu->gpr[31]);
    }
    cpu->cycle_observation_suffix = 0u;
    cpu->pc = 0x80249208u;
    if (gw_over(cpu) || cpu->exception != 0u || (cpu->cycle_budget > 0 && cpu->downcount <= -cpu->cycle_budget) ||
        !bw_direct_call_ready(cpu, 0x80249208u))
        return GW_STOP;

    /* 80249208 (11): the arguments to r27-r30; the pass check; the virtual target; bctrl */
    if (!gw_ram(cpu, cpu->gpr[4] + 4u, 4u) || !gw_ram(cpu, cpu->gpr[3] + 4u, 4u))
        return GW_STOP; /* pc 0x80249208 */
    {
        const u32 vtable = bw_mem_read32(cpu, cpu->gpr[3] + 4u);
        if (!gw_ram(cpu, vtable + 32u, 4u))
            return GW_STOP;
    }
    cpu->downcount -= 11;
    cpu->gpr[28] = cpu->gpr[3];
    cpu->gpr[29] = cpu->gpr[4];
    cpu->gpr[27] = cpu->gpr[5];
    cpu->gpr[30] = cpu->gpr[6];
    cpu->gpr[4] = cpu->gpr[27];
    cpu->gpr[5] = bw_mem_read32(cpu, cpu->gpr[29] + 4u);
    cpu->gpr[12] = bw_mem_read32(cpu, cpu->gpr[3] + 4u);
    cpu->gpr[12] = bw_mem_read32(cpu, cpu->gpr[12] + 32u);
    cpu->ctr = cpu->gpr[12];
    {
        const u32 target = cpu->ctr & ~3u;
        cpu->lr = 0x80249230u;
        cpu->pc = target;
        if (target != GW_FILTER || !bw_direct_call_ready(cpu, target) || !bluewake_native_bg(cpu, GW_FILTER))
            return GW_STOP; /* the dispatcher runs the target from pc */
        /* The leaf returned: pc = 0x80249230. */
        if (!bw_direct_call_ready(cpu, 0x80249230u))
            return GW_STOP;
    }

    /* 80249230 (2): rlwinm. r0,r3,0,24,31; beq 80249240 */
    cpu->downcount -= 2;
    cpu->gpr[0] = cpu->gpr[3] & 0xFFu;
    gw_cr0_record(cpu, cpu->gpr[0]);
    if (cpu->gpr[0] != 0u) {
        /* 80249238 (2): li r3,0; b 80249350 */
        cpu->downcount -= 2;
        cpu->gpr[3] = 0u;
        goto epilogue;
    }

    /* 80249240 (7): the bounds record; lfs f0,4(r3); lfs f1,36(r29); fcmpo; bgt 80249284 */
    {
        if (!gw_ram(cpu, cpu->gpr[28] + 156u, 4u)) {
            cpu->pc = 0x80249240u;
            return GW_STOP;
        }
        const u32 bounds = bw_mem_read32(cpu, cpu->gpr[28] + 156u) + ((cpu->gpr[27] << 5) & 0xFFFFFFE0u);
        if (!gw_ram(cpu, bounds + 4u, 24u) || !gw_ram(cpu, cpu->gpr[29] + 36u, 20u)) {
            cpu->pc = 0x80249240u;
            return GW_STOP;
        }
    }
    cpu->downcount -= 7;
    cpu->gpr[3] = bw_mem_read32(cpu, cpu->gpr[28] + 156u);
    cpu->gpr[0] = (cpu->gpr[27] << 5) & 0xFFFFFFE0u;
    cpu->gpr[3] = cpu->gpr[3] + cpu->gpr[0];
    gw_lfs(cpu, 0, cpu->gpr[3] + 4u, 3u);
    gw_lfs(cpu, 1, cpu->gpr[29] + 36u, 2u);
    gw_fcmpo(cpu, cpu->fpr[0], cpu->fpr[1]);
    if (cpu->cr & 0x40000000u)
        goto outside_284;
    /* 8024925C (3): lfs f0,16(r3); fcmpo; blt 80249284 */
    cpu->downcount -= 3;
    gw_lfs(cpu, 0, cpu->gpr[3] + 16u, 2u);
    gw_fcmpo(cpu, cpu->fpr[0], cpu->fpr[1]);
    if (cpu->cr & 0x80000000u)
        goto outside_284;
    /* 80249268 (4): lfs f0,12(r3); lfs f1,44(r29); fcmpo; bgt 80249284 */
    cpu->downcount -= 4;
    gw_lfs(cpu, 0, cpu->gpr[3] + 12u, 3u);
    gw_lfs(cpu, 1, cpu->gpr[29] + 44u, 2u);
    gw_fcmpo(cpu, cpu->fpr[0], cpu->fpr[1]);
    if (cpu->cr & 0x40000000u)
        goto outside_284;
    /* 80249278 (3): lfs f0,24(r3); fcmpo; bnl 8024928C */
    cpu->downcount -= 3;
    gw_lfs(cpu, 0, cpu->gpr[3] + 24u, 2u);
    gw_fcmpo(cpu, cpu->fpr[0], cpu->fpr[1]);
    if ((cpu->cr & 0x80000000u) == 0u) {
        /* 8024928C (1): li r0,1 */
        cpu->downcount -= 1;
        cpu->gpr[0] = 1u;
    } else {
    outside_284:
        /* 80249284 (2): li r0,0; b 80249290 */
        cpu->downcount -= 2;
        cpu->gpr[0] = 0u;
    }
    /* 80249290 (2): rlwinm. r0,r0,0,24,31; beq 802492B8 */
    cpu->downcount -= 2;
    cpu->gpr[0] &= 0xFFu;
    gw_cr0_record(cpu, cpu->gpr[0]);
    if (cpu->gpr[0] == 0u)
        goto reject_2b8;
    /* 80249298 (4): lfs f1,8(r3); lfs f0,40(r29); fcmpo f1,f0; bnl 802492B8 */
    cpu->downcount -= 4;
    gw_lfs(cpu, 1, cpu->gpr[3] + 8u, 3u);
    gw_lfs(cpu, 0, cpu->gpr[29] + 40u, 2u);
    gw_fcmpo(cpu, cpu->fpr[1], cpu->fpr[0]);
    if ((cpu->cr & 0x80000000u) == 0u)
        goto reject_2b8;
    /* 802492A8 (4): lfs f1,20(r3); lfs f0,52(r29); fcmpo f1,f0; bnl 802492C0 */
    cpu->downcount -= 4;
    gw_lfs(cpu, 1, cpu->gpr[3] + 20u, 3u);
    gw_lfs(cpu, 0, cpu->gpr[29] + 52u, 2u);
    gw_fcmpo(cpu, cpu->fpr[1], cpu->fpr[0]);
    if ((cpu->cr & 0x80000000u) == 0u)
        goto inside_2c0;
reject_2b8:
    /* 802492B8 (2): li r3,0; b 80249350 */
    cpu->downcount -= 2;
    cpu->gpr[3] = 0u;
    goto epilogue;

inside_2c0:
    /* 802492C0 (10): li r31,0; the group table entry; lhz r5,46 (its tree); cmplwi r5,0xFFFF; beq 802492F8 */
    {
        if (!gw_ram(cpu, cpu->gpr[28] + 148u, 4u)) {
            cpu->pc = 0x802492C0u;
            return GW_STOP;
        }
        const u32 bgd = bw_mem_read32(cpu, cpu->gpr[28] + 148u);
        if (!gw_ram(cpu, bgd + 36u, 4u)) {
            cpu->pc = 0x802492C0u;
            return GW_STOP;
        }
        const u32 entry = bw_mem_read32(cpu, bgd + 36u) + (u32)((s64)(s32)cpu->gpr[27] * 52);
        if (!gw_ram(cpu, entry + 46u, 2u)) {
            cpu->pc = 0x802492C0u;
            return GW_STOP;
        }
        if (bw_mem_read16(cpu, entry + 46u) != 0xFFFFu) {
            /* A polygon tree: GroundCrossRp stays translated, from this block. */
            cpu->pc = 0x802492C0u;
            return GW_STOP;
        }
    }
    cpu->downcount -= 10;
    cpu->gpr[31] = 0u;
    cpu->gpr[3] = bw_mem_read32(cpu, cpu->gpr[28] + 148u);
    cpu->gpr[0] = bw_mem_read32(cpu, cpu->gpr[3] + 36u);
    cpu->gpr[27] = (u32)((s64)(s32)cpu->gpr[27] * 52);
    cpu->gpr[3] = cpu->gpr[0] + cpu->gpr[27];
    cpu->gpr[5] = bw_mem_read16(cpu, cpu->gpr[3] + 46u);
    gw_cr0_logical(cpu, cpu->gpr[5], 0xFFFFu);

    /* 802492F8 (4): the first child: lhz r27,40 */
    {
        const u32 bgd = bw_mem_read32(cpu, cpu->gpr[28] + 148u);
        const u32 table = gw_ram(cpu, bgd + 36u, 4u) ? bw_mem_read32(cpu, bgd + 36u) : 0u;
        if (!gw_ram(cpu, bgd + 36u, 4u) || !gw_ram(cpu, table + cpu->gpr[27] + 40u, 2u)) {
            cpu->pc = 0x802492F8u;
            return GW_STOP;
        }
    }
    cpu->downcount -= 4;
    cpu->gpr[3] = bw_mem_read32(cpu, cpu->gpr[28] + 148u);
    cpu->gpr[0] = bw_mem_read32(cpu, cpu->gpr[3] + 36u);
    cpu->gpr[3] = cpu->gpr[0] + cpu->gpr[27];
    cpu->cycle_observation_suffix = 0u;
    cpu->gpr[27] = mem_read16(cpu, cpu->gpr[3] + 40u);

    for (;;) {
        /* 80249308 (3, precharge): addis r0,r27,0; cmplwi r0,0xFFFF; beq 8024934C */
        if (!gw_precharge(cpu, 0x80249308u, 3u))
            return GW_STOP;
        cpu->gpr[0] = cpu->gpr[27];
        gw_cr0_logical(cpu, cpu->gpr[0], 0xFFFFu);
        if (cpu->gpr[0] == 0xFFFFu)
            break;
        /* 80249314 (5, precharge): the child's arguments; bl 802491F4 */
        if (!gw_precharge(cpu, 0x80249314u, 5u))
            return GW_STOP;
        cpu->gpr[3] = cpu->gpr[28];
        cpu->gpr[4] = cpu->gpr[29];
        cpu->gpr[5] = cpu->gpr[27];
        cpu->gpr[6] = cpu->gpr[30] + 1u;
        cpu->lr = GW_CHILD_RETURN;
        if (gw_over(cpu)) {
            cpu->pc = BLUEWAKE_GROUND_WALK_ENTRY;
            return GW_STOP;
        }
        if (gw_walk(cpu, depth + 1u) != GW_RETURN)
            return GW_STOP;
        /* The child's blr: the chunk's return dispatch checks the budget, then 80249328. */
        if (gw_over(cpu))
            return GW_STOP; /* pc 0x80249328 */
        /* 80249328 (2, precharge): rlwinm. r0,r3,0,24,31; beq 80249334 */
        if (!gw_precharge(cpu, GW_CHILD_RETURN, 2u))
            return GW_STOP;
        cpu->gpr[0] = cpu->gpr[3] & 0xFFu;
        gw_cr0_record(cpu, cpu->gpr[0]);
        if (cpu->gpr[0] != 0u) {
            /* 80249330 (1, precharge): li r31,1 */
            if (!gw_precharge(cpu, 0x80249330u, 1u))
                return GW_STOP;
            cpu->gpr[31] = 1u;
        }
        /* 80249334 (8): the next sibling: lhzx r27 at table + 52 * r27 + 38; b 80249308 */
        {
            if (!gw_ram(cpu, cpu->gpr[28] + 148u, 4u)) {
                cpu->pc = 0x80249334u;
                return GW_STOP;
            }
            const u32 bgd = bw_mem_read32(cpu, cpu->gpr[28] + 148u);
            if (!gw_ram(cpu, bgd + 36u, 4u)) {
                cpu->pc = 0x80249334u;
                return GW_STOP;
            }
            const u32 table = bw_mem_read32(cpu, bgd + 36u);
            if (!gw_ram(cpu, table + (u32)((s64)(s32)cpu->gpr[27] * 52) + 38u, 2u)) {
                cpu->pc = 0x80249334u;
                return GW_STOP;
            }
        }
        cpu->downcount -= 8;
        cpu->gpr[3] = bw_mem_read32(cpu, cpu->gpr[28] + 148u);
        cpu->gpr[4] = bw_mem_read32(cpu, cpu->gpr[3] + 36u);
        cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[27] * 52);
        cpu->gpr[0] = cpu->gpr[3] + 38u;
        cpu->gpr[27] = bw_mem_read16(cpu, cpu->gpr[4] + cpu->gpr[0]);
        if (gw_over(cpu)) {
            cpu->pc = 0x80249308u;
            return GW_STOP;
        }
    }
    /* 8024934C (1): or r3,r31,r31 */
    cpu->downcount -= 1;
    cpu->gpr[3] = cpu->gpr[31];

epilogue:
    /* 80249350 (2): addi r11,r1,32; bl _restgpr_27, inline */
    cpu->downcount -= 2;
    cpu->gpr[11] = cpu->gpr[1] + 32u;
    cpu->lr = 0x80249358u;
    cpu->pc = GW_RESTORE;
    if (!bw_direct_call_ready(cpu, GW_RESTORE) || !bw_inline_gpr_memory_ready(cpu, 27u) ||
        !(cpu->downcount - 5 > -cpu->cycle_budget))
        return GW_STOP;
    cpu->downcount -= 6;
    {
        u8* const slots = bw_inline_gpr_frame(cpu, 27u, false);
        cpu->gpr[27] = read_be32(slots + 0);
        cpu->gpr[28] = read_be32(slots + 4);
        cpu->gpr[29] = read_be32(slots + 8);
        cpu->gpr[30] = read_be32(slots + 12);
        cpu->gpr[31] = read_be32(slots + 16);
    }
    cpu->cycle_observation_suffix = 0u;
    cpu->pc = 0x80249358u;
    if (gw_over(cpu) || cpu->exception != 0u || (cpu->cycle_budget > 0 && cpu->downcount <= -cpu->cycle_budget) ||
        !bw_direct_call_ready(cpu, 0x80249358u))
        return GW_STOP;
    /* 80249358 (5): lwz r0,36(r1); mtlr r0; addi r1,r1,32; blr */
    if (!gw_ram(cpu, cpu->gpr[1] + 36u, 4u))
        return GW_STOP; /* pc 0x80249358 */
    cpu->downcount -= 5;
    cpu->gpr[0] = bw_mem_read32(cpu, cpu->gpr[1] + 36u);
    cpu->lr = cpu->gpr[0];
    cpu->gpr[1] += 32u;
    cpu->pc = cpu->lr & ~3u;
    return GW_RETURN;
}

int bluewake_ground_walk(CPUState* cpu) {
    if (cpu == NULL || cpu->ram == NULL || cpu->exception != 0u || g_mem_write_journal != NULL ||
        g_ppc_guest_aliases_overlap_mem1 || cpu->reserve_valid || (cpu->msr & PPC_MSR_FP) == 0u ||
        cpu->cycle_budget <= 0 || gw_short(cpu) ||
        (cpu->cycle_deadline_budget > 0 && cpu->cycle_deadline_budget < 16) ||
        !bw_edge_filter_enabled || !bw_edge_watch_ready || !bw_host_quiet(cpu) || !gw_boundaries_unwatched() ||
        /* The first block's own checks: a stop there would change nothing and
         * send the dispatcher straight back to this hook. */
        !gw_store(cpu, cpu->gpr[1] - 32u, 4u) || !gw_store(cpu, cpu->gpr[1] + 4u, 4u)) {
        s_declined++;
        return GW_DECLINE;
    }
    const int result = gw_walk(cpu, 0u);
    if (result == GW_RETURN)
        s_returned++;
    else
        s_stopped++;
    return result;
}

int bluewake_ground_walk_try(CPUState* cpu) {
    if (cpu == NULL || s_ready == NULL || !s_ready(s_ready_user, cpu, BLUEWAKE_GROUND_WALK_ENTRY))
        return GW_DECLINE;
    return bluewake_ground_walk(cpu);
}
