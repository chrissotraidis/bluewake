/* Two collision checks (GZLE01 cBgS_Chk::ChkSameActorPid and
 * dBgW::ChkGrpThrough), native.
 *
 * Both are small integer functions the collision code calls per polygon
 * group and per check: ChkSameActorPid (four blocks) 0.24 percent of the game
 * thread on Outset, ChkGrpThrough (a virtual call, 0.64 percent) more, and
 * the translation of ChkGrpThrough is split between two chunks: its blocks
 * from 0x800A96E0 on (the water, lava, poison and light group tests) are in
 * the next chunk, so every call that reaches them leaves its chunk at the
 * boundary, goes back through the chassis loop to run the rest, and returns
 * through the loop again.
 *
 * Each runs its blocks in order on the same values - the loads, the compares
 * and record forms into CR0 (with XER's SO), the cycles each block charges -
 * and leaves every register as the translation does: r0, r3, r4 and r5 as
 * the last instruction to write them left them, CR, the cycles, pc at the
 * return address. The prepaid copies' RAM loads store no cycle suffix; the
 * one block ChkGrpThrough has no prepaid copy of (0x800A96DC, the last in its
 * chunk) stores zero, and so does the native on that path.
 *
 * It declines, changing nothing, unless that is certain: every load plain
 * RAM, no exception pending, no write journal, the turn's budget not spent at
 * any block's entry, and the next deadline beyond each block (so the
 * translation prepays every block and never leaves its copy). A path across
 * ChkGrpThrough's chunk boundary also needs the boundary to pass silently,
 * as it does in play: the module's edge filter on (direct_calls.h: the host
 * handed over its flags and the builder's watch list), the host's edge
 * service quiet, and the boundary address (0x800A96E0 or 0x800A96F0) not one
 * the host watches - then neither the direct call that entered the function
 * nor the chassis loop between the chunks asks the host anything, and the
 * function's result is the same however its blocks are reached.
 *
 * tests/native_bg_test.c compares both with the translation, every register
 * and byte. No identifier here may be `ctx`. */
#include "native_bg.h"
#include "direct_calls.h"

#include <stdio.h>

int bluewake_native_bg_enabled;

enum { BG_SAME_ACTOR, BG_GRP_THROUGH, BG_GROUND_CROSS_GRP, BG_COUNT };
static unsigned long long s_bg_runs[BG_COUNT], s_bg_declined[BG_COUNT], s_bg_crossed;

void bluewake_native_bg_report(void) {
    fprintf(stderr,
            "[native-bg] same-actor-pid=%llu/%llu grp-through=%llu/%llu ground-cross-grp=%llu/%llu "
            "(native/declined; %llu across the chunk boundary)\n",
            s_bg_runs[0], s_bg_declined[0], s_bg_runs[1], s_bg_declined[1],
            s_bg_runs[2], s_bg_declined[2], s_bg_crossed);
}

/* One block's entry as the translation makes it, on a running downcount:
 * false where it would stop for the turn's budget, or not prepay the block,
 * or leave its prepaid copy at a deadline (an access's suffix beyond it). */
static inline bool bg_block(const CPUState* cpu, s64* downcount, s64 cycles) {
    if (*downcount <= -cpu->cycle_budget)
        return false;
    if (cpu->cycle_deadline_budget > 0 &&
        (cpu->cycle_deadline_budget < cycles || cpu->cycle_deadline_budget + *downcount < cycles))
        return false;
    *downcount -= cycles;
    return true;
}

static inline bool bg_ram(const CPUState* cpu, u32 address, u32 size) {
    return ppc_dispatch_poll_read_stable((CPUState*)cpu, address, size);
}

static inline u32 bg_word(const CPUState* cpu, u32 address) {
    return read_be32(cpu->ram + (address - GC_RAM_BASE));
}

/* CR0 from a compare (or a record form's result against zero), with SO. */
static inline u32 bg_cr0(u32 cr, u32 xer, bool less, bool greater) {
    const u32 bits = (less ? 0x8u : greater ? 0x4u : 0x2u) | ((xer >> 31) & 1u);
    return (cr & 0x0FFFFFFFu) | (bits << 28);
}

static inline u32 bg_cr0_signed(u32 cr, u32 xer, s32 a, s32 b) { return bg_cr0(cr, xer, a < b, a > b); }
static inline u32 bg_cr0_unsigned(u32 cr, u32 xer, u32 a, u32 b) { return bg_cr0(cr, xer, a < b, a > b); }

static inline bool bg_ready(const CPUState* cpu) {
    return cpu != NULL && cpu->ram != NULL && cpu->exception == 0u &&
           g_mem_write_journal == NULL && cpu->cycle_budget > 0;
}

/* cBgS_Chk::ChkSameActorPid(r3 this, r4 pid): this->mActorPid (8) and
 * this->mSameActorChk (12, a byte). */
static int bg_same_actor(CPUState* cpu) {
    if (!bg_ready(cpu))
        return 0;
    const u32 xer = cpu->xer;
    s64 downcount = cpu->downcount;
    const u32 self = cpu->gpr[3], pid = cpu->gpr[4];
    u32 r0, r3, r5, cr;
    /* 8024734C (4): lwz r5,8(r3); addis r0,r5,1; cmplwi r0,0xFFFF; beq 80247374 */
    if (!bg_block(cpu, &downcount, 4) || !bg_ram(cpu, self + 8u, 4))
        return 0;
    r5 = bg_word(cpu, self + 8u);
    r0 = r5 + 0x10000u;
    cr = bg_cr0_unsigned(cpu->cr, xer, r0, 0xFFFFu);
    if (r0 != 0xFFFFu) {
        /* 8024735C (3): addis r0,r4,1; cmplwi r0,0xFFFF; beq 80247374 */
        if (!bg_block(cpu, &downcount, 3))
            return 0;
        r0 = pid + 0x10000u;
        cr = bg_cr0_unsigned(cr, xer, r0, 0xFFFFu);
        if (r0 != 0xFFFFu) {
            /* 80247368 (3): lbz r0,12(r3); cmplwi r0,0; bne 8024737C */
            if (!bg_block(cpu, &downcount, 3) || !bg_ram(cpu, self + 12u, 1))
                return 0;
            r0 = cpu->ram[self + 12u - GC_RAM_BASE];
            cr = bg_cr0_unsigned(cr, xer, r0, 0u);
            if (r0 != 0u) {
                /* 8024737C (4): subf r0,r5,r4; cntlzw r0,r0; rlwinm r3,r0,27,24,31; blr */
                if (!bg_block(cpu, &downcount, 4))
                    return 0;
                const u32 difference = pid - r5;
                r0 = difference == 0u ? 32u : (u32)__builtin_clz(difference);
                r3 = ((r0 << 27) | (r0 >> 5)) & 0xFFu;
                goto done;
            }
        }
    }
    /* 80247374 (2): li r3,0; blr */
    if (!bg_block(cpu, &downcount, 2))
        return 0;
    r3 = 0u;
done:
    cpu->gpr[0] = r0;
    cpu->gpr[3] = r3;
    cpu->gpr[5] = r5;
    cpu->cr = cr;
    cpu->downcount = downcount;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}

/* The chunk boundary inside ChkGrpThrough passes without the host: the edge
 * filter's own test (dispatch_loop.h), which a direct call's return path
 * (direct_calls.h) only narrows. */
static inline bool bg_boundary_silent(const CPUState* cpu, u32 address) {
    return bw_edge_filter_enabled && bw_edge_watch_ready && bw_host_quiet(cpu) && bw_edge_unwatched(address);
}

/* dBgW::ChkGrpThrough(r3 this, r4 group, r5 pass check, r6 depth):
 * this->pm_bgd (148) -> m_g_tbl (36) -> [group].m_info (52-byte entries, at
 * 48), against the pass check's mask flags (4). */
static int bg_grp_through(CPUState* cpu) {
    if (!bg_ready(cpu))
        return 0;
    const u32 xer = cpu->xer;
    s64 downcount = cpu->downcount;
    const u32 check = cpu->gpr[5], depth = cpu->gpr[6];
    u32 r0 = cpu->gpr[0], r3 = cpu->gpr[3], r4 = cpu->gpr[4], cr;
    u32 boundary = 0u;
    bool suffix_zero = false;
    u32 flags = 0u;
    /* 800A9684 (2): cmpwi r6,2; bne 800A9694 */
    if (!bg_block(cpu, &downcount, 2))
        return 0;
    cr = bg_cr0_signed(cpu->cr, xer, (s32)depth, 2);
    if (depth != 2u)
        goto false_9694;
    /* 800A968C (2): cmplwi r5,0; bne 800A969C */
    if (!bg_block(cpu, &downcount, 2))
        return 0;
    cr = bg_cr0_unsigned(cr, xer, check, 0u);
    if (check == 0u)
        goto false_9694;
    /* 800A969C (11): lwz r3,148(r3); lwz r3,36(r3); mulli r0,r4,52; add r3,r3,r0;
     * lwz r4,48(r3); lis r3,8; addi r0,r3,1792; and. r0,r4,r0; bne 800A96D4 */
    if (!bg_block(cpu, &downcount, 11) || !bg_ram(cpu, r3 + 148u, 4))
        return 0;
    r3 = bg_word(cpu, r3 + 148u);
    if (!bg_ram(cpu, r3 + 36u, 4))
        return 0;
    r3 = bg_word(cpu, r3 + 36u);
    r0 = r4 * 52u;
    r3 += r0;
    if (!bg_ram(cpu, r3 + 48u, 4) || !bg_ram(cpu, check + 4u, 4))
        return 0;
    r4 = bg_word(cpu, r3 + 48u);
    flags = bg_word(cpu, check + 4u); /* every later load of the pass check's flags reads this word */
    r3 = 0x00080000u;
    r0 = r4 & 0x00080700u;
    cr = bg_cr0_signed(cr, xer, (s32)r0, 0);
    if (r0 == 0u) {
        /* 800A96C0 (3): lwz r0,4(r5); rlwinm. r0,r0,0,31,31; beq 800A96D4 */
        if (!bg_block(cpu, &downcount, 3))
            return 0;
        r0 = flags & 1u;
        cr = bg_cr0_signed(cr, xer, (s32)r0, 0);
        if (r0 != 0u)
            goto false_96CC;
    }
    /* 800A96D4 (2): rlwinm. r0,r4,0,23,23; beq 800A96F0 (the next chunk) */
    if (!bg_block(cpu, &downcount, 2))
        return 0;
    r0 = r4 & 0x100u;
    cr = bg_cr0_signed(cr, xer, (s32)r0, 0);
    if (r0 != 0u) {
        /* 800A96DC (1, no prepaid copy; suffix 0): lwz r0,4(r5); the chunk ends */
        if (!bg_block(cpu, &downcount, 1))
            return 0;
        r0 = flags;
        suffix_zero = true;
        boundary = 0x800A96E0u;
        /* 800A96E0 (2): rlwinm. r0,r0,0,30,30; beq 800A96F0 */
        if (!bg_block(cpu, &downcount, 2))
            return 0;
        r0 &= 2u;
        cr = bg_cr0_signed(cr, xer, (s32)r0, 0);
        if (r0 != 0u) {
            /* 800A96E8 (2): li r3,0; blr */
            if (!bg_block(cpu, &downcount, 2))
                return 0;
            r3 = 0u;
            goto done;
        }
    } else {
        boundary = 0x800A96F0u;
    }
    /* 800A96F0, 800A970C, 800A9728 (2 each): rlwinm. r0,r4 with the group's
     * bit; beq to the next test. 800A96F8, 800A9714, 800A9730 (3 each):
     * lwz r0,4(r5); rlwinm. r0,r0 with the check's bit; beq to the next test.
     * 800A9704, 800A9720, 800A973C (2 each): li r3,0; blr. */
    {
        static const u32 group_bits[3] = {0x200u, 0x400u, 0x80000u};
        static const u32 check_bits[3] = {0x4u, 0x8u, 0x10u};
        for (unsigned test = 0; test < 3; ++test) {
            if (!bg_block(cpu, &downcount, 2))
                return 0;
            r0 = r4 & group_bits[test];
            cr = bg_cr0_signed(cr, xer, (s32)r0, 0);
            if (r0 == 0u)
                continue;
            if (!bg_block(cpu, &downcount, 3))
                return 0;
            r0 = flags & check_bits[test];
            cr = bg_cr0_signed(cr, xer, (s32)r0, 0);
            if (r0 == 0u)
                continue;
            if (!bg_block(cpu, &downcount, 2))
                return 0;
            r3 = 0u;
            goto done;
        }
    }
    /* 800A9744 (2): li r3,1; blr */
    if (!bg_block(cpu, &downcount, 2))
        return 0;
    r3 = 1u;
    goto done;
false_96CC:
    /* 800A96CC (2): li r3,0; blr */
    if (!bg_block(cpu, &downcount, 2))
        return 0;
    r3 = 0u;
    goto done;
false_9694:
    /* 800A9694 (2): li r3,0; blr */
    if (!bg_block(cpu, &downcount, 2))
        return 0;
    r3 = 0u;
done:
    if (boundary != 0u) {
        if (!bg_boundary_silent(cpu, boundary))
            return 0;
        s_bg_crossed++;
    }
    cpu->gpr[0] = r0;
    cpu->gpr[3] = r3;
    cpu->gpr[4] = r4;
    cpu->cr = cr;
    cpu->downcount = downcount;
    if (suffix_zero)
        cpu->cycle_observation_suffix = 0u;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}

/* cBgW::GroundCrossGrpRp, for the complete path on which the virtual
 * ChkGrpThrough call filters the group. This path never reaches the mutating
 * GroundCrossRp call or child recursion. Everything is prepared in a local
 * CPU image; stack bytes are committed only after the whole path is proven. */
static int bg_ground_cross_grp_early(CPUState* cpu) {
    if (!bg_ready(cpu) || g_ppc_guest_aliases_overlap_mem1)
        return 0;

    const u32 old_sp = cpu->gpr[1];
    const u32 frame = old_sp - 32u;
    const u32 self = cpu->gpr[3];
    const u32 check = cpu->gpr[4];
    const u32 group = cpu->gpr[5];
    const u32 depth = cpu->gpr[6];
    const u32 old_lr = cpu->lr;
    const u32 saved[5] = {cpu->gpr[27], cpu->gpr[28], cpu->gpr[29], cpu->gpr[30], cpu->gpr[31]};

    if (!bg_ram(cpu, frame, 4u) || !bg_ram(cpu, old_sp + 4u, 4u) ||
        !bg_ram(cpu, old_sp - 20u, 20u) || !bg_ram(cpu, check + 4u, 4u) ||
        !bg_ram(cpu, self + 4u, 4u))
        return 0;

    const u32 vtable = bg_word(cpu, self + 4u);
    if (!bg_ram(cpu, vtable + 32u, 4u))
        return 0;
    const u32 target = bg_word(cpu, vtable + 32u) & ~3u;
    if (target != BLUEWAKE_BG_CHK_GRP_THROUGH ||
        !bw_direct_call_ready(cpu, 0x80328F38u) ||
        !bw_direct_call_ready(cpu, 0x80249208u) ||
        !bw_direct_call_ready(cpu, target) ||
        !bw_direct_call_ready(cpu, 0x80249230u))
        return 0;

    CPUState work = *cpu;
    if (!bg_block(&work, &work.downcount, 5u))
        return 0;
    work.gpr[1] = frame;
    work.gpr[0] = old_lr;
    work.gpr[11] = old_sp;

    if (!bg_block(&work, &work.downcount, 6u))
        return 0;
    work.lr = 0x80249208u;
    work.pc = 0x80249208u;
    work.cycle_observation_suffix = 0u;

    if (!bg_block(&work, &work.downcount, 11u))
        return 0;
    work.gpr[28] = self;
    work.gpr[29] = check;
    work.gpr[27] = group;
    work.gpr[30] = depth;
    work.gpr[4] = group;
    work.gpr[5] = bg_word(cpu, check + 4u);
    work.gpr[12] = target;
    work.ctr = target;
    work.lr = 0x80249230u;
    work.pc = target;
    if (!bg_grp_through(&work) || work.pc != 0x80249230u)
        return 0;

    if (!bg_block(&work, &work.downcount, 2u))
        return 0;
    work.gpr[0] = work.gpr[3] & 0xFFu;
    work.cr = bg_cr0_signed(work.cr, work.xer, (s32)work.gpr[0], 0);
    if (work.gpr[0] != 0u) {
        /* Filtered group: li r3,0; b epilogue. */
        if (!bg_block(&work, &work.downcount, 2u))
            return 0;
        work.gpr[3] = 0u;
    } else {
        /* First bounds rejection: the lower bound is already above the test
         * point. Later bounds/tree/child paths still decline unchanged. */
        if (!bg_ram(cpu, self + 156u, 4u))
            return 0;
        const u32 bounds = bg_word(cpu, self + 156u) + group * 32u;
        if ((work.msr & PPC_MSR_FP) == 0u || (work.fpscr & 3u) != 0u ||
            !bg_ram(cpu, bounds + 4u, 4u) || !bg_ram(cpu, check + 36u, 4u) ||
            !bg_block(&work, &work.downcount, 7u))
            return 0;
        work.gpr[3] = bounds;
        work.gpr[0] = group * 32u;
        work.fpr[0] = f64_value(convert_to_double(bg_word(cpu, bounds + 4u)));
        work.fpr[1] = f64_value(convert_to_double(bg_word(cpu, check + 36u)));
        work.ps1[0] = work.fpr[0];
        work.ps1[1] = work.fpr[1];
        ppc_fcmp(&work, 0, work.fpr[0], work.fpr[1], true);
        if ((work.cr & 0x40000000u) == 0u)
            return 0;
        if (!bg_block(&work, &work.downcount, 2u))
            return 0;
        work.gpr[0] = 0u;
        if (!bg_block(&work, &work.downcount, 2u))
            return 0;
        work.cr = bg_cr0_signed(work.cr, work.xer, 0, 0);
        if (!bg_block(&work, &work.downcount, 2u))
            return 0;
        work.gpr[3] = 0u;
    }

    if (!bg_block(&work, &work.downcount, 2u) ||
        !bg_block(&work, &work.downcount, 6u) ||
        !bg_block(&work, &work.downcount, 5u))
        return 0;
    work.gpr[11] = old_sp;
    for (unsigned i = 0; i < 5; ++i)
        work.gpr[27 + i] = saved[i];
    work.gpr[0] = old_lr;
    work.lr = old_lr;
    work.gpr[1] = old_sp;
    work.pc = old_lr & ~3u;
    work.cycle_observation_suffix = 0u;

    *cpu = work;
    write_be32(cpu->ram + (frame - GC_RAM_BASE), old_sp);
    write_be32(cpu->ram + (old_sp + 4u - GC_RAM_BASE), old_lr);
    for (unsigned i = 0; i < 5; ++i)
        write_be32(cpu->ram + (old_sp - 20u + i * 4u - GC_RAM_BASE), saved[i]);
    clear_matching_reservation(cpu, frame);
    clear_matching_reservation(cpu, old_sp + 4u);
    for (unsigned i = 0; i < 5; ++i)
        clear_matching_reservation(cpu, old_sp - 20u + i * 4u);
    return 1;
}

int bluewake_native_bg(CPUState* cpu, u32 address) {
    unsigned which;
    int done;
    switch (address) {
    case BLUEWAKE_BG_CHK_SAME_ACTOR_PID: which = BG_SAME_ACTOR; done = bg_same_actor(cpu); break;
    case BLUEWAKE_BG_CHK_GRP_THROUGH: which = BG_GRP_THROUGH; done = bg_grp_through(cpu); break;
    case BLUEWAKE_BG_GROUND_CROSS_GRP_RP:
        which = BG_GROUND_CROSS_GRP;
        done = bg_ground_cross_grp_early(cpu);
        break;
    default: return 0;
    }
    if (done)
        s_bg_runs[which]++;
    else
        s_bg_declined[which]++;
    return done;
}
