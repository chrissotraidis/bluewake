/* GZLE01 game math, native: cXyz arithmetic wrappers, cylinder/AABB overlap,
 * mDoMtx rotation setters, J3DUClipper and bounded J3D animation paths. The decomp explains their purpose;
 * the Windows translation defines every operation, register write, stack byte and cycle.
 *
 * Ordinary RAM only, no aliases/journal, aligned and disjoint in/out/stack,
 * FP enabled, unquantised pairs, nearest rounding, finite bounded operands,
 * and enough turn/deadline budget for every block. All checks precede stores.
 * Arithmetic uses inline_fp.h (NI, 25-bit multipliers, FPRF and FI/FR), loads
 * duplicate singles into both FPR halves, and stores invalidate reservations.
 * Calls to PSVEC leaves reproduce their full state, including scratch FPRs.
 * The last observation suffix is kept even when later instructions do not
 * observe cycles. tests/native_game_math_test.c compares the complete state
 * and every RAM byte against the personal DLL. No identifier may be `ctx`.
 */
#include "native_game_math.h"
#include "inline_fp.h"

#include <stdio.h>

int bluewake_native_game_math_enabled;
enum {
    GM_ADD,
    GM_SUB,
    GM_SCALE,
    GM_CYL,
    GM_XROT,
    GM_YROT,
    GM_ZROT,
    GM_BOX,
    GM_SPHERE_CLIP,
    GM_BOX_CLIP,
    GM_KEY,
    GM_TRANSFORM,
    GM_COUNT
};
static unsigned long long runs[GM_COUNT], declines[GM_COUNT];

void bluewake_native_game_math_report(void) {
    static const char* names[] = {"xyz-add",     "xyz-sub",  "xyz-scale", "aab-cyl",
                                  "xrotS",       "yrotS",    "zrotS",     "box-line",
                                  "sphere-clip", "box-clip", "key-s",     "transform-simple"};
    fprintf(stderr, "[native-game-math]");
    for (unsigned i = 0; i < GM_COUNT; ++i)
        fprintf(stderr, " %s=%llu/%llu", names[i], runs[i], declines[i]);
    fprintf(stderr, " (native/declined)\n");
}

static bool ram_ok(const CPUState* cpu, u32 at, u32 bytes) {
    return cpu->ram != NULL && (at & 3u) == 0u && ppc_dispatch_poll_read_stable((CPUState*)cpu, at, bytes);
}

static bool apart(u32 a, u32 na, u32 b, u32 nb) { return (u64)a + na <= b || (u64)b + nb <= a; }

/* Zeros or normal singles in [2^-60,2^30). Products stay normal and finite;
 * cancellation/NI rounding is performed by the translation's own helpers. */
static bool bounded(u32 bits) {
    u32 m = bits & 0x7FFFFFFFu;
    return m == 0u || (m >= 0x21800000u && m < 0x4E800000u);
}

static u32 word(const CPUState* cpu, u32 at) { return read_be32(cpu->ram + (at - GC_RAM_BASE)); }
static f64 gm_single(u32 bits) { return f64_value(convert_to_double(bits)); }
static u32 gm_single_bits(f64 value) { return convert_to_single(f64_bits(value)); }
static u16 gm_word16(const CPUState* cpu, u32 at) { return read_be16(cpu->ram + at - GC_RAM_BASE); }
static void gm_short_load(CPUState* cpu, unsigned r, u32 at) {
    cpu->fpr[r] = (f64)(s16)gm_word16(cpu, at);
    cpu->ps1[r] = 1.0;
}
static u32 gm_rotl32(u32 value, unsigned n) { return n ? (value << n) | (value >> (32u - n)) : value; }
static u64 gm_word64(const CPUState* cpu, u32 at) { return read_be64(cpu->ram + at - GC_RAM_BASE); }

static bool floats_ok(const CPUState* cpu, u32 at, unsigned count) {
    if (!ram_ok(cpu, at, count * 4u))
        return false;
    for (unsigned i = 0; i < count; ++i)
        if (!bounded(word(cpu, at + i * 4u)))
            return false;
    return true;
}

/* Conservative whole-function bound: decline whenever a boundary or an
 * observed access could see the turn/deadline expire. Avoid signed overflow. */
static bool ready(const CPUState* cpu, s64 bound, bool pairs) {
    if (cpu->exception || !(cpu->msr & PPC_MSR_FP) || (cpu->fpscr & 3u) || g_mem_write_journal != NULL ||
        cpu->cycle_budget <= bound || cpu->downcount < -cpu->cycle_budget + bound || cpu->downcount > 0)
        return false;
    if (pairs && (!(cpu->hid2 & PPC_HID2_LSQE) || (cpu->gqr[0] & 0x00070007u)))
        return false;
    return cpu->cycle_deadline_budget <= 0 ||
           (cpu->cycle_deadline_budget >= bound && cpu->downcount >= bound - cpu->cycle_deadline_budget);
}

static void store(CPUState* cpu, u32 at, u32 bits) {
    clear_matching_reservation(cpu, at);
    write_be32(cpu->ram + (at - GC_RAM_BASE), bits);
}
static void gm_store64(CPUState* cpu, u32 at, u64 bits) {
    clear_matching_reservation(cpu, at);
    write_be64(cpu->ram + at - GC_RAM_BASE, bits);
}
static void gm_store16(CPUState* cpu, u32 at, u16 bits) {
    clear_matching_reservation(cpu, at);
    write_be16(cpu->ram + at - GC_RAM_BASE, bits);
}
static void gm_psq_store(CPUState* cpu, unsigned r, u32 at) {
    store(cpu, at, convert_to_single_ftz(f64_bits(cpu->fpr[r])));
    store(cpu, at + 4u, convert_to_single_ftz(f64_bits(cpu->ps1[r])));
}
static void gm_psq_load(CPUState* cpu, unsigned r, u32 at) {
    cpu->fpr[r] = gm_single(word(cpu, at));
    cpu->ps1[r] = gm_single(word(cpu, at + 4u));
}
static void gm_pair(CPUState* cpu, unsigned r, u32 at, bool w) {
    cpu->fpr[r] = gm_single(word(cpu, at));
    cpu->ps1[r] = w ? 1.0 : gm_single(word(cpu, at + 4u));
}

static void lfs(CPUState* cpu, unsigned r, u32 at) {
    cpu->fpr[r] = cpu->ps1[r] = f64_value(convert_to_double(word(cpu, at)));
}

static void stfs(CPUState* cpu, unsigned r, u32 at) {
    store(cpu, at, convert_to_single(f64_bits(cpu->fpr[r])));
}

static int finish(CPUState* cpu, s64 cycles, u32 suffix) {
    cpu->downcount -= cycles;
    cpu->cycle_observation_suffix = suffix;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}

/* PSMTXMultVec's 21 instructions on preflighted ranges. The ps_sum0
 * destination's old second half is rounded/carried, just as in the DLL. */
static void gm_multvec(CPUState* cpu) {
    const u32 m = cpu->gpr[3], v = cpu->gpr[4], out = cpu->gpr[5];
    gm_pair(cpu, 0, v, false);
    gm_pair(cpu, 1, v + 8u, true);
    for (unsigned row = 0; row < 3; ++row) {
        const unsigned base = row == 1 ? 8 : 2, dest = row == 1 ? 12 : 6;
        gm_pair(cpu, base, m + row * 16u, false);
        gm_pair(cpu, base + 1u, m + row * 16u + 8u, false);
        bw_fp_ps_mul_op(cpu, base + 2u, base, 0);
        bw_fp_ps_madd_op(cpu, base + 3u, base + 1u, 1, base + 2u, false, false);
        bw_fp_ps_sum0(cpu, dest, base + 3u, dest, base + 3u);
        store(cpu, out + 4u * row, convert_to_single_ftz(f64_bits(cpu->fpr[dest])));
    }
    finish(cpu, 21, 1);
}

/* Hidden result r3, this r4, rhs r5 (or scale f1). */
static int xyz(CPUState* cpu, unsigned kind) {
    const u32 sp = cpu->gpr[1], frame = sp - 32u;
    const u32 out = cpu->gpr[3], a = cpu->gpr[4], b = cpu->gpr[5];
    const s64 cycles = kind == GM_SCALE ? 27 : 30;
    if (!ready(cpu, cycles, true) || !ram_ok(cpu, frame, 40) || !ram_ok(cpu, out, 12) ||
        !floats_ok(cpu, a, 3) || !apart(frame, 40, a, 12) || !apart(frame, 40, out, 12) ||
        !apart(a, 12, out, 12))
        return 0;
    if (kind != GM_SCALE && (!floats_ok(cpu, b, 3) || !apart(frame, 40, b, 12) || !apart(b, 12, out, 12)))
        return 0;
    if (kind == GM_SCALE) {
        const u64 mag = f64_bits(cpu->fpr[1]) & 0x7FFFFFFFFFFFFFFFull;
        if (mag != 0 && (mag < 0x3C30000000000000ull || mag >= 0x41D0000000000000ull))
            return 0;
    }
    store(cpu, frame, sp);
    const u32 saved_lr = cpu->lr, saved_r31 = cpu->gpr[31];
    store(cpu, frame + 36u, saved_lr);
    store(cpu, frame + 28u, saved_r31);
    cpu->gpr[1] = frame;
    cpu->gpr[31] = out;
    cpu->gpr[3] = a;
    cpu->gpr[4] = kind == GM_SCALE ? frame + 8u : b;
    if (kind != GM_SCALE)
        cpu->gpr[5] = frame + 8u;
    cpu->lr = kind == GM_ADD ? 0x80245698u : kind == GM_SUB ? 0x802456E8u : 0x80245734u;
    /* The SDK's instructions, on the preflighted ranges. Preserve both
     * halves of every scratch FPR; no nested guard can fail after stores. */
    if (kind == GM_SCALE) {
        gm_pair(cpu, 0, a, false);
        gm_pair(cpu, 2, a + 8u, true);
        bw_fp_ps_muls0(cpu, 0, 0, 1);
        gm_psq_store(cpu, 0, frame + 8u);
        bw_fp_ps_muls0(cpu, 0, 2, 1);
        store(cpu, frame + 16u, convert_to_single_ftz(f64_bits(cpu->fpr[0])));
    } else {
        gm_pair(cpu, 2, a, false);
        gm_pair(cpu, 4, b, false);
        if (kind == GM_SUB)
            bw_fp_ps_sub_op(cpu, 6, 2, 4);
        else
            bw_fp_ps_add_op(cpu, 6, 2, 4);
        gm_psq_store(cpu, 6, frame + 8u);
        gm_pair(cpu, 3, a + 8u, true);
        gm_pair(cpu, 5, b + 8u, true);
        if (kind == GM_SUB)
            bw_fp_ps_sub_op(cpu, 7, 3, 5);
        else
            bw_fp_ps_add_op(cpu, 7, 3, 5);
        store(cpu, frame + 16u, convert_to_single_ftz(f64_bits(cpu->fpr[7])));
    }
    for (unsigned i = 0; i < 3; ++i) {
        lfs(cpu, 0, frame + 8u + 4u * i);
        stfs(cpu, 0, out + 4u * i);
    }
    cpu->gpr[31] = saved_r31;
    cpu->gpr[0] = saved_lr;
    cpu->gpr[1] = sp;
    cpu->lr = saved_lr;
    return finish(cpu, cycles, 2);
}

/* The six ordered slab checks, with the short-circuit path's FPRs and cycles. */
static int aab_cyl(CPUState* cpu) {
    const u32 a = cpu->gpr[3], c = cpu->gpr[4];
    if (!ready(cpu, 32, false) || !floats_ok(cpu, a, 6) || !floats_ok(cpu, c, 5))
        return 0;
    lfs(cpu, 1, a);
    lfs(cpu, 2, c);
    lfs(cpu, 3, c + 12u);
    bw_fp_fadds(cpu, 0, 2, 3);
    bw_fp_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);
    s64 cycles = 6;
    u32 suffix = 3;
    if (cpu->cr & 0x40000000u)
        goto no;
    cycles += 4;
    suffix = 3;
    lfs(cpu, 1, a + 12u);
    bw_fp_fsubs(cpu, 0, 2, 3);
    bw_fp_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);
    if (cpu->cr & 0x80000000u)
        goto no;
    cycles += 5;
    suffix = 3;
    lfs(cpu, 1, a + 8u);
    lfs(cpu, 2, c + 8u);
    bw_fp_fadds(cpu, 0, 2, 3);
    bw_fp_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);
    if (cpu->cr & 0x40000000u)
        goto no;
    cycles += 4;
    suffix = 3;
    lfs(cpu, 1, a + 20u);
    bw_fp_fsubs(cpu, 0, 2, 3);
    bw_fp_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);
    if (cpu->cr & 0x80000000u)
        goto no;
    cycles += 6;
    suffix = 3;
    lfs(cpu, 1, a + 4u);
    lfs(cpu, 2, c + 4u);
    lfs(cpu, 0, c + 16u);
    bw_fp_fadds(cpu, 0, 2, 0);
    bw_fp_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);
    if (cpu->cr & 0x40000000u)
        goto no;
    lfs(cpu, 0, a + 16u);
    bw_fp_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[2], true);
    cpu->gpr[0] = cpu->cr & 0x80000000u ? 31u : 32u;
    cpu->gpr[3] = cpu->gpr[0] >> 5;
    return finish(cpu, cycles + 7, 6);
no:
    cpu->gpr[3] = 0;
    return finish(cpu, cycles + 2, suffix);
}

/* SDA table pointers and the shift are guest data, never host sin/cos. */
static int rot_s(CPUState* cpu, unsigned axis) {
    const u32 out = cpu->gpr[3], sda = cpu->gpr[13] - 26460u;
    const u32 constants = cpu->gpr[2] - 32584u;
    if (!ready(cpu, 24, false) || !ram_ok(cpu, sda, 12) || !ram_ok(cpu, out, 48) ||
        !floats_ok(cpu, constants, 2) || !apart(out, 48, constants, 8) || !apart(out, 48, sda, 12))
        return 0;
    const u32 shift = word(cpu, sda) & 63u;
    const u32 index = shift >= 32u ? 0u : ((cpu->gpr[4] & 0xFFFFu) >> shift) * 4u;
    const u32 sin_base = word(cpu, sda + 4u), cos_base = word(cpu, sda + 8u);
    const u32 sn = sin_base + index, cs = cos_base + index;
    if (!floats_ok(cpu, sn, 1) || !floats_ok(cpu, cs, 1) || !apart(out, 48, sn, 4) || !apart(out, 48, cs, 4))
        return 0;
    cpu->gpr[5] = cos_base;
    cpu->gpr[4] = sin_base;
    cpu->gpr[0] = index;
    /* Positive masked input to sraw cannot set CA. */
    cpu->xer &= ~0x20000000u;
    lfs(cpu, 2, cs);
    lfs(cpu, 3, sn);
    lfs(cpu, 1, constants + 4u);
    lfs(cpu, 0, constants);
    const f64 one = cpu->fpr[0], zero = cpu->fpr[1], cosine = cpu->fpr[2], sine = cpu->fpr[3];
    const f64 neg_sine = f64_value(f64_bits(sine) ^ 0x8000000000000000ull);
    f64 m[12];
    if (axis == 0) {
        f64 v[] = {one, zero, zero, zero, zero, cosine, neg_sine, zero, zero, sine, cosine, zero};
        memcpy(m, v, sizeof m);
    } else if (axis == 1) {
        f64 v[] = {cosine, zero, sine, zero, zero, one, zero, zero, neg_sine, zero, cosine, zero};
        memcpy(m, v, sizeof m);
    } else {
        f64 v[] = {cosine, neg_sine, zero, zero, sine, cosine, zero, zero, zero, zero, one, zero};
        memcpy(m, v, sizeof m);
    }
    for (unsigned i = 0; i < 12; ++i)
        store(cpu, out + 4u * i, convert_to_single(f64_bits(m[i])));
    cpu->fpr[0] = axis == 2 ? one : neg_sine; /* fneg preserves PS1 and FPSCR */
    return finish(cpu, 24, 1);
}

/* Early slab returns of cM3d_Cross_MinMaxBoxLine. The prefix executes on
 * a private CPU image and reads only validated RAM; bevel/plane work declines.
 * Only a completed prefix commits the prologue's 15 saved FP pairs and GPRs.
 * Stack PS1 halves are single-rounded on restore, including non-single bits. */
static int box_line(CPUState* original) {
    const u32 frame = original->gpr[1] - 512u;
    const u32 masks = original->gpr[2] - 16512u;
    if (!ready(original, 256, true) || (frame & 7u) || !ram_ok(original, frame, 520) ||
        !ram_ok(original, masks, 24) || !apart(frame, 520, masks, 24))
        return 0;
    for (unsigned r = 3; r <= 6; ++r)
        if (!floats_ok(original, original->gpr[r], 3) || !apart(frame, 520, original->gpr[r], 12))
            return 0;
    CPUState state = *original;
    CPUState* cpu = &state;
    cpu->downcount -= 39; /* entry block 35, _savegpr_29 four precise cycles */
    cpu->downcount -= 8;
    // 8024AEC8: or   r29, r3, r3
    {
        cpu->gpr[29] = cpu->gpr[3] | cpu->gpr[3];
    }

    // 8024AECC: or   r30, r4, r4
    {
        cpu->gpr[30] = cpu->gpr[4] | cpu->gpr[4];
    }

    // 8024AED0: li      r0, 0
    cpu->gpr[0] = (u32)(s32)(0);

    // 8024AED4: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 8024AED8: lfs     f1, 0(r5)
    lfs(cpu, 1, cpu->gpr[5] + (u32)(s32)(0));

    // 8024AEDC: lfs     f0, 0(r4)
    lfs(cpu, 0, cpu->gpr[4] + (u32)(s32)(0));

    // 8024AEE0: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 8024AEE4: bc    4, 1, 0x8024AF04
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024AF04;

    cpu->downcount -= 3;
    // 8024AEE8: lfs     f2, 0(r6)
    lfs(cpu, 2, cpu->gpr[6] + (u32)(s32)(0));

    // 8024AEEC: fcmpo   cr0, f2, f0
    ppc_fcmp(cpu, 0, cpu->fpr[2], cpu->fpr[0], true);

    // 8024AEF0: bc    4, 1, 0x8024AEFC
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024AEFC;

    cpu->downcount -= 2;
    // 8024AEF4: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 8024AEF8: b       0x8024B988
    {
        goto box_done;
    }

label_8024AEFC:
    cpu->downcount -= 2;
    // 8024AEFC: lwz     r0, -16512(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16512);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 8024AF00: b       0x8024AF14
    {
        goto label_8024AF14;
    }

label_8024AF04:
    cpu->downcount -= 3;
    // 8024AF04: lfs     f2, 0(r6)
    lfs(cpu, 2, cpu->gpr[6] + (u32)(s32)(0));

    // 8024AF08: fcmpo   cr0, f2, f0
    ppc_fcmp(cpu, 0, cpu->fpr[2], cpu->fpr[0], true);

    // 8024AF0C: bc    4, 1, 0x8024AF14
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024AF14;

    cpu->downcount -= 1;
    // 8024AF10: lwz     r3, -16512(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16512);
        cpu->gpr[3] = word(cpu, ea);
    }

label_8024AF14:
    cpu->downcount -= 2;
    // 8024AF14: rlwinm. r4, r0, 0, 31, 31
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[0], 0u) & 0x00000001u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024AF18: bc    4, 2, 0x8024AF50
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024AF50;

    cpu->downcount -= 3;
    // 8024AF1C: lfs     f3, 0(r29)
    lfs(cpu, 3, cpu->gpr[29] + (u32)(s32)(0));

    // 8024AF20: fcmpo   cr0, f1, f3
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[3], true);

    // 8024AF24: bc    4, 0, 0x8024AF50
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024AF50;

    cpu->downcount -= 2;
    // 8024AF28: rlwinm. r4, r3, 0, 31, 31
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[3], 0u) & 0x00000001u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024AF2C: bc    4, 2, 0x8024AF44
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024AF44;

    cpu->downcount -= 3;
    // 8024AF30: lfs     f2, 0(r6)
    lfs(cpu, 2, cpu->gpr[6] + (u32)(s32)(0));

    // 8024AF34: fcmpo   cr0, f2, f3
    ppc_fcmp(cpu, 0, cpu->fpr[2], cpu->fpr[3], true);

    // 8024AF38: bc    4, 0, 0x8024AF44
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024AF44;

    cpu->downcount -= 2;
    // 8024AF3C: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 8024AF40: b       0x8024B988
    {
        goto box_done;
    }

label_8024AF44:
    cpu->downcount -= 3;
    // 8024AF44: lwz     r4, -16508(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16508);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024AF48: or   r0, r0, r4
    {
        cpu->gpr[0] = cpu->gpr[0] | cpu->gpr[4];
    }

    // 8024AF4C: b       0x8024AF70
    {
        goto label_8024AF70;
    }

label_8024AF50:
    cpu->downcount -= 2;
    // 8024AF50: rlwinm. r4, r3, 0, 31, 31
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[3], 0u) & 0x00000001u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024AF54: bc    4, 2, 0x8024AF70
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024AF70;

    cpu->downcount -= 4;
    // 8024AF58: lfs     f3, 0(r6)
    lfs(cpu, 3, cpu->gpr[6] + (u32)(s32)(0));

    // 8024AF5C: lfs     f2, 0(r29)
    lfs(cpu, 2, cpu->gpr[29] + (u32)(s32)(0));

    // 8024AF60: fcmpo   cr0, f3, f2
    ppc_fcmp(cpu, 0, cpu->fpr[3], cpu->fpr[2], true);

    // 8024AF64: bc    4, 0, 0x8024AF70
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024AF70;

    cpu->downcount -= 2;
    // 8024AF68: lwz     r4, -16508(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16508);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024AF6C: or   r3, r3, r4
    {
        cpu->gpr[3] = cpu->gpr[3] | cpu->gpr[4];
    }

label_8024AF70:
    cpu->downcount -= 4;
    // 8024AF70: lfs     f3, 8(r5)
    lfs(cpu, 3, cpu->gpr[5] + (u32)(s32)(8));

    // 8024AF74: lfs     f2, 8(r30)
    lfs(cpu, 2, cpu->gpr[30] + (u32)(s32)(8));

    // 8024AF78: fcmpo   cr0, f3, f2
    ppc_fcmp(cpu, 0, cpu->fpr[3], cpu->fpr[2], true);

    // 8024AF7C: bc    4, 1, 0x8024AFA0
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024AFA0;

    cpu->downcount -= 3;
    // 8024AF80: lfs     f4, 8(r6)
    lfs(cpu, 4, cpu->gpr[6] + (u32)(s32)(8));

    // 8024AF84: fcmpo   cr0, f4, f2
    ppc_fcmp(cpu, 0, cpu->fpr[4], cpu->fpr[2], true);

    // 8024AF88: bc    4, 1, 0x8024AF94
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024AF94;

    cpu->downcount -= 2;
    // 8024AF8C: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 8024AF90: b       0x8024B988
    {
        goto box_done;
    }

label_8024AF94:
    cpu->downcount -= 3;
    // 8024AF94: lwz     r4, -16504(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16504);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024AF98: or   r0, r0, r4
    {
        cpu->gpr[0] = cpu->gpr[0] | cpu->gpr[4];
    }

    // 8024AF9C: b       0x8024AFB4
    {
        goto label_8024AFB4;
    }

label_8024AFA0:
    cpu->downcount -= 3;
    // 8024AFA0: lfs     f4, 8(r6)
    lfs(cpu, 4, cpu->gpr[6] + (u32)(s32)(8));

    // 8024AFA4: fcmpo   cr0, f4, f2
    ppc_fcmp(cpu, 0, cpu->fpr[4], cpu->fpr[2], true);

    // 8024AFA8: bc    4, 1, 0x8024AFB4
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024AFB4;

    cpu->downcount -= 2;
    // 8024AFAC: lwz     r4, -16504(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16504);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024AFB0: or   r3, r3, r4
    {
        cpu->gpr[3] = cpu->gpr[3] | cpu->gpr[4];
    }

label_8024AFB4:
    cpu->downcount -= 2;
    // 8024AFB4: rlwinm. r4, r0, 0, 27, 27
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[0], 0u) & 0x00000010u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024AFB8: bc    4, 2, 0x8024AFF0
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024AFF0;

    cpu->downcount -= 3;
    // 8024AFBC: lfs     f5, 8(r29)
    lfs(cpu, 5, cpu->gpr[29] + (u32)(s32)(8));

    // 8024AFC0: fcmpo   cr0, f3, f5
    ppc_fcmp(cpu, 0, cpu->fpr[3], cpu->fpr[5], true);

    // 8024AFC4: bc    4, 0, 0x8024AFF0
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024AFF0;

    cpu->downcount -= 2;
    // 8024AFC8: rlwinm. r4, r3, 0, 27, 27
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[3], 0u) & 0x00000010u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024AFCC: bc    4, 2, 0x8024AFE4
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024AFE4;

    cpu->downcount -= 3;
    // 8024AFD0: lfs     f4, 8(r6)
    lfs(cpu, 4, cpu->gpr[6] + (u32)(s32)(8));

    // 8024AFD4: fcmpo   cr0, f4, f5
    ppc_fcmp(cpu, 0, cpu->fpr[4], cpu->fpr[5], true);

    // 8024AFD8: bc    4, 0, 0x8024AFE4
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024AFE4;

    cpu->downcount -= 2;
    // 8024AFDC: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 8024AFE0: b       0x8024B988
    {
        goto box_done;
    }

label_8024AFE4:
    cpu->downcount -= 3;
    // 8024AFE4: lwz     r4, -16500(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16500);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024AFE8: or   r0, r0, r4
    {
        cpu->gpr[0] = cpu->gpr[0] | cpu->gpr[4];
    }

    // 8024AFEC: b       0x8024B010
    {
        goto label_8024B010;
    }

label_8024AFF0:
    cpu->downcount -= 2;
    // 8024AFF0: rlwinm. r4, r3, 0, 27, 27
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[3], 0u) & 0x00000010u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024AFF4: bc    4, 2, 0x8024B010
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024B010;

    cpu->downcount -= 4;
    // 8024AFF8: lfs     f5, 8(r6)
    lfs(cpu, 5, cpu->gpr[6] + (u32)(s32)(8));

    // 8024AFFC: lfs     f4, 8(r29)
    lfs(cpu, 4, cpu->gpr[29] + (u32)(s32)(8));

    // 8024B000: fcmpo   cr0, f5, f4
    ppc_fcmp(cpu, 0, cpu->fpr[5], cpu->fpr[4], true);

    // 8024B004: bc    4, 0, 0x8024B010
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024B010;

    cpu->downcount -= 2;
    // 8024B008: lwz     r4, -16500(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16500);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024B00C: or   r3, r3, r4
    {
        cpu->gpr[3] = cpu->gpr[3] | cpu->gpr[4];
    }

label_8024B010:
    cpu->downcount -= 4;
    // 8024B010: lfs     f4, 4(r5)
    lfs(cpu, 4, cpu->gpr[5] + (u32)(s32)(4));

    // 8024B014: lfs     f11, 4(r30)
    lfs(cpu, 11, cpu->gpr[30] + (u32)(s32)(4));

    // 8024B018: fcmpo   cr0, f4, f11
    ppc_fcmp(cpu, 0, cpu->fpr[4], cpu->fpr[11], true);

    // 8024B01C: bc    4, 1, 0x8024B040
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024B040;

    cpu->downcount -= 3;
    // 8024B020: lfs     f5, 4(r6)
    lfs(cpu, 5, cpu->gpr[6] + (u32)(s32)(4));

    // 8024B024: fcmpo   cr0, f5, f11
    ppc_fcmp(cpu, 0, cpu->fpr[5], cpu->fpr[11], true);

    // 8024B028: bc    4, 1, 0x8024B034
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024B034;

    cpu->downcount -= 2;
    // 8024B02C: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 8024B030: b       0x8024B988
    {
        goto box_done;
    }

label_8024B034:
    cpu->downcount -= 3;
    // 8024B034: lwz     r4, -16496(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16496);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024B038: or   r0, r0, r4
    {
        cpu->gpr[0] = cpu->gpr[0] | cpu->gpr[4];
    }

    // 8024B03C: b       0x8024B054
    {
        goto label_8024B054;
    }

label_8024B040:
    cpu->downcount -= 3;
    // 8024B040: lfs     f5, 4(r6)
    lfs(cpu, 5, cpu->gpr[6] + (u32)(s32)(4));

    // 8024B044: fcmpo   cr0, f5, f11
    ppc_fcmp(cpu, 0, cpu->fpr[5], cpu->fpr[11], true);

    // 8024B048: bc    4, 1, 0x8024B054
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_8024B054;

    cpu->downcount -= 2;
    // 8024B04C: lwz     r4, -16496(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16496);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024B050: or   r3, r3, r4
    {
        cpu->gpr[3] = cpu->gpr[3] | cpu->gpr[4];
    }

label_8024B054:
    cpu->downcount -= 2;
    // 8024B054: rlwinm. r4, r0, 0, 29, 29
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[0], 0u) & 0x00000004u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024B058: bc    4, 2, 0x8024B090
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024B090;

    cpu->downcount -= 3;
    // 8024B05C: lfs     f6, 4(r29)
    lfs(cpu, 6, cpu->gpr[29] + (u32)(s32)(4));

    // 8024B060: fcmpo   cr0, f4, f6
    ppc_fcmp(cpu, 0, cpu->fpr[4], cpu->fpr[6], true);

    // 8024B064: bc    4, 0, 0x8024B090
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024B090;

    cpu->downcount -= 2;
    // 8024B068: rlwinm. r4, r3, 0, 29, 29
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[3], 0u) & 0x00000004u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024B06C: bc    4, 2, 0x8024B084
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024B084;

    cpu->downcount -= 3;
    // 8024B070: lfs     f5, 4(r6)
    lfs(cpu, 5, cpu->gpr[6] + (u32)(s32)(4));

    // 8024B074: fcmpo   cr0, f5, f6
    ppc_fcmp(cpu, 0, cpu->fpr[5], cpu->fpr[6], true);

    // 8024B078: bc    4, 0, 0x8024B084
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024B084;

    cpu->downcount -= 2;
    // 8024B07C: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 8024B080: b       0x8024B988
    {
        goto box_done;
    }

label_8024B084:
    cpu->downcount -= 3;
    // 8024B084: lwz     r4, -16492(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16492);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024B088: or   r0, r0, r4
    {
        cpu->gpr[0] = cpu->gpr[0] | cpu->gpr[4];
    }

    // 8024B08C: b       0x8024B0B0
    {
        goto label_8024B0B0;
    }

label_8024B090:
    cpu->downcount -= 2;
    // 8024B090: rlwinm. r4, r3, 0, 29, 29
    {
        cpu->gpr[4] = gm_rotl32(cpu->gpr[3], 0u) & 0x00000004u;
        u32 cr_bits = 0;
        s32 cr_value = (s32)cpu->gpr[4];
        if (cr_value < 0)
            cr_bits |= 0x8u;
        if (cr_value > 0)
            cr_bits |= 0x4u;
        if (cr_value == 0)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & 0x0FFFFFFFu) | (cr_bits << 28);
    }

    // 8024B094: bc    4, 2, 0x8024B0B0
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024B0B0;

    cpu->downcount -= 4;
    // 8024B098: lfs     f6, 4(r6)
    lfs(cpu, 6, cpu->gpr[6] + (u32)(s32)(4));

    // 8024B09C: lfs     f5, 4(r29)
    lfs(cpu, 5, cpu->gpr[29] + (u32)(s32)(4));

    // 8024B0A0: fcmpo   cr0, f6, f5
    ppc_fcmp(cpu, 0, cpu->fpr[6], cpu->fpr[5], true);

    // 8024B0A4: bc    4, 0, 0x8024B0B0
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_8024B0B0;

    cpu->downcount -= 2;
    // 8024B0A8: lwz     r4, -16492(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-16492);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 8024B0AC: or   r3, r3, r4
    {
        cpu->gpr[3] = cpu->gpr[3] | cpu->gpr[4];
    }

label_8024B0B0:
    cpu->downcount -= 2;
    // 8024B0B0: cmplwi  r0, 0x0000
    {
        u32 val_a = (u32)(cpu->gpr[0]);
        u32 val_b = (u32)(0x0000u);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 8024B0B4: bc    4, 2, 0x8024B0C0
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024B0C0;

    cpu->downcount -= 2;
    // 8024B0B8: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 8024B0BC: b       0x8024B988
    {
        goto box_done;
    }

label_8024B0C0:
    cpu->downcount -= 2;
    // 8024B0C0: cmplwi  r3, 0x0000
    {
        u32 val_a = (u32)(cpu->gpr[3]);
        u32 val_b = (u32)(0x0000u);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 8024B0C4: bc    4, 2, 0x8024B0D0
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_8024B0D0;

    cpu->downcount -= 2;
    // 8024B0C8: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 8024B0CC: b       0x8024B988
    {
        goto box_done;
    }

label_8024B0D0:
    return 0; /* both endpoint outcodes nonzero: leave everything untouched */
box_done:
    store(cpu, frame, original->gpr[1]);
    store(cpu, frame + 516u, original->lr);
    for (unsigned k = 0; k < 15; ++k) {
        const unsigned r = 31u - k;
        const u32 at = frame + 496u - 16u * k;
        clear_matching_reservation(cpu, at);
        write_be64(cpu->ram + at - GC_RAM_BASE, f64_bits(original->fpr[r]));
        store(cpu, at + 8u, convert_to_single_ftz(f64_bits(original->fpr[r])));
        const u32 second = convert_to_single_ftz(f64_bits(original->ps1[r]));
        store(cpu, at + 12u, second);
        cpu->ps1[r] = gm_single(second);
    }
    for (unsigned r = 29; r < 32; ++r)
        store(cpu, frame + 4u * r + 144u, original->gpr[r]);
    cpu->gpr[29] = original->gpr[29];
    cpu->gpr[30] = original->gpr[30];
    cpu->gpr[31] = original->gpr[31];
    cpu->gpr[11] = frame + 272u;
    cpu->gpr[0] = original->lr;
    finish(cpu, 41, 2); /* epilogue block 32, _restgpr_29 four, return block 5 */
    *original = state;
    return 1;
}

/* J3DUClipper::sphere_clip: same instruction order and scalar rounding as
 * the translation; trusted PSMTXMultVec preserves its paired scratch FPRs. */
static int sphere_clip(CPUState* cpu) {
    const u32 frame = cpu->gpr[1] - 48u;
    const u32 self = cpu->gpr[3], matrix = cpu->gpr[4], a = cpu->gpr[5];
    if (!ready(cpu, 160, true) || (frame & 7u) || !ram_ok(cpu, frame, 56) || !floats_ok(cpu, self + 4u, 12) ||
        !floats_ok(cpu, self + 84u, 2) || !floats_ok(cpu, matrix, 12) || !floats_ok(cpu, a, 3) ||
        !apart(frame, 56, self, 92) || !apart(frame, 56, matrix, 48) || !apart(frame, 56, a, 12))
        return 0;
    const u64 radius = f64_bits(cpu->fpr[1]) & 0x7FFFFFFFFFFFFFFFull;
    if (radius != 0 && (radius < 0x3C30000000000000ull || radius >= 0x41D0000000000000ull))
        return 0;

    cpu->downcount -= 12u;
    // 80256888: stwu     r1, -48(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(-48);
        store(cpu, ea, (u32)cpu->gpr[1]);
        cpu->gpr[1] = ea;
    }

    // 8025688C: mflr    r0
    cpu->gpr[0] = cpu->lr;

    // 80256890: stw     r0, 52(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(52);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 80256894: stfd     f31, 32(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(32);
        gm_store64(cpu, ea, f64_bits(cpu->fpr[31]));
    }

    // 80256898: psq_st   f31, 40(r1), 0, 0
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(40);
        gm_psq_store(cpu, 31u, ea);
    }

    // 8025689C: stw     r31, 28(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(28);
        store(cpu, ea, (u32)cpu->gpr[31]);
    }

    // 802568A0: or   r31, r3, r3
    {
        cpu->gpr[31] = cpu->gpr[3] | cpu->gpr[3];
    }

    // 802568A4: fmr    f31, f1
    cpu->fpr[31] = cpu->fpr[1];

    // 802568A8: or   r3, r4, r4
    {
        cpu->gpr[3] = cpu->gpr[4] | cpu->gpr[4];
    }

    // 802568AC: or   r4, r5, r5
    {
        cpu->gpr[4] = cpu->gpr[5] | cpu->gpr[5];
    }

    // 802568B0: addi    r5, r1, 8
    cpu->gpr[5] = cpu->gpr[1] + (u32)(s32)(8);

    // 802568B4: PSMTXMultVec
    cpu->lr = 0x802568B8u;
    gm_multvec(cpu);

    cpu->downcount -= 6u;
    // 802568B8: lfs     f3, 16(r1)
    lfs(cpu, 3, cpu->gpr[1] + (u32)(s32)(16));

    // 802568BC: fneg    f1, f3
    cpu->fpr[1] = f64_value(f64_bits(cpu->fpr[3]) ^ 0x8000000000000000ull);

    // 802568C0: lfs     f0, 84(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(84));

    // 802568C4: fsubs   f0, f0, f31
    ppc_fsubs(cpu, 0, 0, 31);

    // 802568C8: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 802568CC: bc    4, 0, 0x802568D8
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802568D8;

    cpu->downcount -= 2u;
    // 802568D0: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 802568D4: b       0x802569B4
    {
        goto label_802569B4;
    }

label_802568D8:

    cpu->downcount -= 4u;
    // 802568D8: lfs     f0, 88(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(88));

    // 802568DC: fadds   f0, f0, f31
    ppc_fadds(cpu, 0, 0, 31);

    // 802568E0: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 802568E4: bc    4, 1, 0x802568F0
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_802568F0;

    cpu->downcount -= 2u;
    // 802568E8: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 802568EC: b       0x802569B4
    {
        goto label_802569B4;
    }

label_802568F0:

    cpu->downcount -= 12u;
    // 802568F0: lfs     f0, 12(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(12));

    // 802568F4: fmuls   f2, f3, f0
    ppc_fmuls(cpu, 2, 3, 0);

    // 802568F8: lfs     f5, 8(r1)
    lfs(cpu, 5, cpu->gpr[1] + (u32)(s32)(8));

    // 802568FC: lfs     f0, 4(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(4));

    // 80256900: fmuls   f1, f5, f0
    ppc_fmuls(cpu, 1, 5, 0);

    // 80256904: lfs     f4, 12(r1)
    lfs(cpu, 4, cpu->gpr[1] + (u32)(s32)(12));

    // 80256908: lfs     f0, 8(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(8));

    // 8025690C: fmuls   f0, f4, f0
    ppc_fmuls(cpu, 0, 4, 0);

    // 80256910: fadds   f0, f1, f0
    ppc_fadds(cpu, 0, 1, 0);

    // 80256914: fadds   f0, f2, f0
    ppc_fadds(cpu, 0, 2, 0);

    // 80256918: fcmpo   cr0, f0, f31
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[31], true);

    // 8025691C: bc    4, 1, 0x80256928
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_80256928;

    cpu->downcount -= 2u;
    // 80256920: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 80256924: b       0x802569B4
    {
        goto label_802569B4;
    }

label_80256928:

    cpu->downcount -= 10u;
    // 80256928: lfs     f0, 24(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(24));

    // 8025692C: fmuls   f2, f3, f0
    ppc_fmuls(cpu, 2, 3, 0);

    // 80256930: lfs     f0, 16(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(16));

    // 80256934: fmuls   f1, f5, f0
    ppc_fmuls(cpu, 1, 5, 0);

    // 80256938: lfs     f0, 20(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(20));

    // 8025693C: fmuls   f0, f4, f0
    ppc_fmuls(cpu, 0, 4, 0);

    // 80256940: fadds   f0, f1, f0
    ppc_fadds(cpu, 0, 1, 0);

    // 80256944: fadds   f0, f2, f0
    ppc_fadds(cpu, 0, 2, 0);

    // 80256948: fcmpo   cr0, f0, f31
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[31], true);

    // 8025694C: bc    4, 1, 0x80256958
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_80256958;

    cpu->downcount -= 2u;
    // 80256950: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 80256954: b       0x802569B4
    {
        goto label_802569B4;
    }

label_80256958:

    cpu->downcount -= 10u;
    // 80256958: lfs     f0, 36(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(36));

    // 8025695C: fmuls   f2, f3, f0
    ppc_fmuls(cpu, 2, 3, 0);

    // 80256960: lfs     f0, 28(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(28));

    // 80256964: fmuls   f1, f5, f0
    ppc_fmuls(cpu, 1, 5, 0);

    // 80256968: lfs     f0, 32(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(32));

    // 8025696C: fmuls   f0, f4, f0
    ppc_fmuls(cpu, 0, 4, 0);

    // 80256970: fadds   f0, f1, f0
    ppc_fadds(cpu, 0, 1, 0);

    // 80256974: fadds   f0, f2, f0
    ppc_fadds(cpu, 0, 2, 0);

    // 80256978: fcmpo   cr0, f0, f31
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[31], true);

    // 8025697C: bc    4, 1, 0x80256988
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_80256988;

    cpu->downcount -= 2u;
    // 80256980: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 80256984: b       0x802569B4
    {
        goto label_802569B4;
    }

label_80256988:

    cpu->downcount -= 11u;
    // 80256988: lfs     f0, 48(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(48));

    // 8025698C: fmuls   f2, f3, f0
    ppc_fmuls(cpu, 2, 3, 0);

    // 80256990: lfs     f0, 40(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(40));

    // 80256994: fmuls   f1, f5, f0
    ppc_fmuls(cpu, 1, 5, 0);

    // 80256998: lfs     f0, 44(r31)
    lfs(cpu, 0, cpu->gpr[31] + (u32)(s32)(44));

    // 8025699C: fmuls   f0, f4, f0
    ppc_fmuls(cpu, 0, 4, 0);

    // 802569A0: fadds   f0, f1, f0
    ppc_fadds(cpu, 0, 1, 0);

    // 802569A4: fadds   f0, f2, f0
    ppc_fadds(cpu, 0, 2, 0);

    // 802569A8: fcmpo   cr0, f0, f31
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[31], true);

    // 802569AC: mfcr    r0
    cpu->gpr[0] = cpu->cr;

    // 802569B0: rlwinm r3, r0, 2, 31, 31
    {
        cpu->gpr[3] = gm_rotl32(cpu->gpr[0], 2u) & 0x00000001u;
    }

label_802569B4:

    cpu->downcount -= 8u;
    // 802569B4: psq_l   f31, 40(r1), 0, 0
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(40);
        gm_psq_load(cpu, 31u, ea);
    }

    // 802569B8: lfd     f31, 32(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(32);
        cpu->fpr[31] = f64_value(gm_word64(cpu, ea));
    }

    // 802569BC: lwz     r31, 28(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(28);
        cpu->gpr[31] = word(cpu, ea);
    }

    // 802569C0: lwz     r0, 52(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(52);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802569C4: mtlr    r0
    cpu->lr = cpu->gpr[0];

    // 802569C8: addi    r1, r1, 48
    cpu->gpr[1] = cpu->gpr[1] + (u32)(s32)(48);

    // 802569CC: blr
    cpu->cycle_observation_suffix = 2;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}

/* J3DUClipper::box_clip: same instruction order and scalar rounding as
 * the translation; trusted PSMTXMultVec preserves its paired scratch FPRs. */
static int box_clip(CPUState* cpu) {
    const u32 frame = cpu->gpr[1] - 176u;
    const u32 self = cpu->gpr[3], matrix = cpu->gpr[4], a = cpu->gpr[5];
    if (!ready(cpu, 1600, true) || (frame & 7u) || !ram_ok(cpu, frame, 184) ||
        !floats_ok(cpu, self + 4u, 12) || !floats_ok(cpu, self + 84u, 2) || !floats_ok(cpu, matrix, 12) ||
        !floats_ok(cpu, a, 3) || !apart(frame, 184, self, 92) || !apart(frame, 184, matrix, 48) ||
        !apart(frame, 184, a, 12))
        return 0;
    const u32 b = cpu->gpr[6], zero = cpu->gpr[2] - 16184u;
    if (!floats_ok(cpu, b, 3) || !floats_ok(cpu, zero, 1) || !apart(frame, 184, b, 12) ||
        !apart(frame, 184, zero, 4))
        return 0;

    cpu->downcount -= 7u;
    // 802569D0: stwu     r1, -176(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(-176);
        store(cpu, ea, (u32)cpu->gpr[1]);
        cpu->gpr[1] = ea;
    }

    // 802569D4: mflr    r0
    cpu->gpr[0] = cpu->lr;

    // 802569D8: stw     r0, 180(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(180);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802569DC: stfd     f31, 160(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(160);
        gm_store64(cpu, ea, f64_bits(cpu->fpr[31]));
    }

    // 802569E0: psq_st   f31, 168(r1), 0, 0
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(168);
        gm_psq_store(cpu, 31u, ea);
    }

    // 802569E4: addi    r11, r1, 160
    cpu->gpr[11] = cpu->gpr[1] + (u32)(s32)(160);

    // 802569E8: save r28-r31
    cpu->lr = 0x802569ECu;
    cpu->downcount -= 5;
    store(cpu, cpu->gpr[11] - 16u, cpu->gpr[28]);
    store(cpu, cpu->gpr[11] - 12u, cpu->gpr[29]);
    store(cpu, cpu->gpr[11] - 8u, cpu->gpr[30]);
    store(cpu, cpu->gpr[11] - 4u, cpu->gpr[31]);

    cpu->downcount -= 8u;
    // 802569EC: or   r30, r3, r3
    {
        cpu->gpr[30] = cpu->gpr[3] | cpu->gpr[3];
    }

    // 802569F0: or   r31, r4, r4
    {
        cpu->gpr[31] = cpu->gpr[4] | cpu->gpr[4];
    }

    // 802569F4: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 802569F8: or   r7, r3, r3
    {
        cpu->gpr[7] = cpu->gpr[3] | cpu->gpr[3];
    }

    // 802569FC: addi    r4, r1, 20
    cpu->gpr[4] = cpu->gpr[1] + (u32)(s32)(20);

    // 80256A00: li      r0, 6
    cpu->gpr[0] = (u32)(s32)(6);

    // 80256A04: mtctr    r0
    cpu->ctr = cpu->gpr[0];

    /* loop_80256A08: six stwx/addi/bdnz iterations, three cycles each. */
    for (unsigned i = 0; i < 6; ++i)
        store(cpu, cpu->gpr[4] + 4u * i, cpu->gpr[7]);
    cpu->gpr[3] = 24;
    cpu->ctr = 0;
    cpu->downcount -= 18;

    cpu->downcount -= 33u;
    // 80256A14: lfs     f4, 0(r6)
    lfs(cpu, 4, cpu->gpr[6] + (u32)(s32)(0));

    // 80256A18: stfs     f4, 44(r1)
    stfs(cpu, 4, cpu->gpr[1] + (u32)(s32)(44));

    // 80256A1C: lfs     f0, 4(r6)
    lfs(cpu, 0, cpu->gpr[6] + (u32)(s32)(4));

    // 80256A20: stfs     f0, 48(r1)
    stfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(48));

    // 80256A24: lfs     f3, 8(r5)
    lfs(cpu, 3, cpu->gpr[5] + (u32)(s32)(8));

    // 80256A28: stfs     f3, 52(r1)
    stfs(cpu, 3, cpu->gpr[1] + (u32)(s32)(52));

    // 80256A2C: stfs     f4, 56(r1)
    stfs(cpu, 4, cpu->gpr[1] + (u32)(s32)(56));

    // 80256A30: stfs     f0, 60(r1)
    stfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(60));

    // 80256A34: lfs     f2, 8(r6)
    lfs(cpu, 2, cpu->gpr[6] + (u32)(s32)(8));

    // 80256A38: stfs     f2, 64(r1)
    stfs(cpu, 2, cpu->gpr[1] + (u32)(s32)(64));

    // 80256A3C: lfs     f1, 0(r5)
    lfs(cpu, 1, cpu->gpr[5] + (u32)(s32)(0));

    // 80256A40: stfs     f1, 68(r1)
    stfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(68));

    // 80256A44: stfs     f0, 72(r1)
    stfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(72));

    // 80256A48: stfs     f2, 76(r1)
    stfs(cpu, 2, cpu->gpr[1] + (u32)(s32)(76));

    // 80256A4C: stfs     f1, 80(r1)
    stfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(80));

    // 80256A50: stfs     f0, 84(r1)
    stfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(84));

    // 80256A54: stfs     f3, 88(r1)
    stfs(cpu, 3, cpu->gpr[1] + (u32)(s32)(88));

    // 80256A58: stfs     f4, 92(r1)
    stfs(cpu, 4, cpu->gpr[1] + (u32)(s32)(92));

    // 80256A5C: lfs     f0, 4(r5)
    lfs(cpu, 0, cpu->gpr[5] + (u32)(s32)(4));

    // 80256A60: stfs     f0, 96(r1)
    stfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(96));

    // 80256A64: stfs     f3, 100(r1)
    stfs(cpu, 3, cpu->gpr[1] + (u32)(s32)(100));

    // 80256A68: stfs     f4, 104(r1)
    stfs(cpu, 4, cpu->gpr[1] + (u32)(s32)(104));

    // 80256A6C: stfs     f0, 108(r1)
    stfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(108));

    // 80256A70: stfs     f2, 112(r1)
    stfs(cpu, 2, cpu->gpr[1] + (u32)(s32)(112));

    // 80256A74: stfs     f1, 116(r1)
    stfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(116));

    // 80256A78: stfs     f0, 120(r1)
    stfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(120));

    // 80256A7C: stfs     f2, 124(r1)
    stfs(cpu, 2, cpu->gpr[1] + (u32)(s32)(124));

    // 80256A80: stfs     f1, 128(r1)
    stfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(128));

    // 80256A84: stfs     f0, 132(r1)
    stfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(132));

    // 80256A88: stfs     f3, 136(r1)
    stfs(cpu, 3, cpu->gpr[1] + (u32)(s32)(136));

    // 80256A8C: li      r28, 0
    cpu->gpr[28] = (u32)(s32)(0);

    // 80256A90: li      r29, 0
    cpu->gpr[29] = (u32)(s32)(0);

    // 80256A94: lfs     f31, -16184(r2)
    lfs(cpu, 31, cpu->gpr[2] + (u32)(s32)(-16184));

label_80256A98:

    cpu->downcount -= 5u;
    // 80256A98: or   r3, r31, r31
    {
        cpu->gpr[3] = cpu->gpr[31] | cpu->gpr[31];
    }

    // 80256A9C: addi    r4, r1, 44
    cpu->gpr[4] = cpu->gpr[1] + (u32)(s32)(44);

    // 80256AA0: add   r4, r4, r29
    {
        u32 a = cpu->gpr[4];
        u32 b = cpu->gpr[29];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 80256AA4: addi    r5, r1, 8
    cpu->gpr[5] = cpu->gpr[1] + (u32)(s32)(8);

    // 80256AA8: PSMTXMultVec
    cpu->lr = 0x80256AACu;
    gm_multvec(cpu);

    cpu->downcount -= 6u;
    // 80256AAC: li      r4, 0
    cpu->gpr[4] = (u32)(s32)(0);

    // 80256AB0: lfs     f0, 16(r1)
    lfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(16));

    // 80256AB4: fneg    f1, f0
    cpu->fpr[1] = f64_value(f64_bits(cpu->fpr[0]) ^ 0x8000000000000000ull);

    // 80256AB8: lfs     f0, 84(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(84));

    // 80256ABC: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 80256AC0: bc    4, 0, 0x80256AD4
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_80256AD4;

    cpu->downcount -= 4u;
    // 80256AC4: lwz     r3, 36(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(36);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 80256AC8: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 80256ACC: stw     r0, 36(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(36);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 80256AD0: li      r4, 1
    cpu->gpr[4] = (u32)(s32)(1);

label_80256AD4:

    cpu->downcount -= 5u;
    // 80256AD4: lfs     f0, 16(r1)
    lfs(cpu, 0, cpu->gpr[1] + (u32)(s32)(16));

    // 80256AD8: fneg    f1, f0
    cpu->fpr[1] = f64_value(f64_bits(cpu->fpr[0]) ^ 0x8000000000000000ull);

    // 80256ADC: lfs     f0, 88(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(88));

    // 80256AE0: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 80256AE4: bc    4, 1, 0x80256AF8
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_80256AF8;

    cpu->downcount -= 4u;
    // 80256AE8: lwz     r3, 40(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(40);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 80256AEC: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 80256AF0: stw     r0, 40(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(40);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 80256AF4: addi    r4, r4, 1
    cpu->gpr[4] = cpu->gpr[4] + (u32)(s32)(1);

label_80256AF8:

    cpu->downcount -= 13u;
    // 80256AF8: lfs     f1, 16(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(16));

    // 80256AFC: lfs     f0, 12(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(12));

    // 80256B00: fmuls   f3, f1, f0
    ppc_fmuls(cpu, 3, 1, 0);

    // 80256B04: lfs     f4, 8(r1)
    lfs(cpu, 4, cpu->gpr[1] + (u32)(s32)(8));

    // 80256B08: lfs     f0, 4(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(4));

    // 80256B0C: fmuls   f2, f4, f0
    ppc_fmuls(cpu, 2, 4, 0);

    // 80256B10: lfs     f1, 12(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(12));

    // 80256B14: lfs     f0, 8(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(8));

    // 80256B18: fmuls   f0, f1, f0
    ppc_fmuls(cpu, 0, 1, 0);

    // 80256B1C: fadds   f0, f2, f0
    ppc_fadds(cpu, 0, 2, 0);

    // 80256B20: fadds   f0, f3, f0
    ppc_fadds(cpu, 0, 3, 0);

    // 80256B24: fcmpo   cr0, f0, f31
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[31], true);

    // 80256B28: bc    4, 1, 0x80256B3C
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_80256B3C;

    cpu->downcount -= 4u;
    // 80256B2C: lwz     r3, 20(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(20);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 80256B30: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 80256B34: stw     r0, 20(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(20);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 80256B38: addi    r4, r4, 1
    cpu->gpr[4] = cpu->gpr[4] + (u32)(s32)(1);

label_80256B3C:

    cpu->downcount -= 12u;
    // 80256B3C: lfs     f1, 16(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(16));

    // 80256B40: lfs     f0, 24(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(24));

    // 80256B44: fmuls   f3, f1, f0
    ppc_fmuls(cpu, 3, 1, 0);

    // 80256B48: lfs     f0, 16(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(16));

    // 80256B4C: fmuls   f2, f4, f0
    ppc_fmuls(cpu, 2, 4, 0);

    // 80256B50: lfs     f1, 12(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(12));

    // 80256B54: lfs     f0, 20(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(20));

    // 80256B58: fmuls   f0, f1, f0
    ppc_fmuls(cpu, 0, 1, 0);

    // 80256B5C: fadds   f0, f2, f0
    ppc_fadds(cpu, 0, 2, 0);

    // 80256B60: fadds   f0, f3, f0
    ppc_fadds(cpu, 0, 3, 0);

    // 80256B64: fcmpo   cr0, f0, f31
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[31], true);

    // 80256B68: bc    4, 1, 0x80256B7C
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_80256B7C;

    cpu->downcount -= 4u;
    // 80256B6C: lwz     r3, 24(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(24);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 80256B70: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 80256B74: stw     r0, 24(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(24);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 80256B78: addi    r4, r4, 1
    cpu->gpr[4] = cpu->gpr[4] + (u32)(s32)(1);

label_80256B7C:

    cpu->downcount -= 13u;
    // 80256B7C: lfs     f1, 16(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(16));

    // 80256B80: lfs     f0, 36(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(36));

    // 80256B84: fmuls   f3, f1, f0
    ppc_fmuls(cpu, 3, 1, 0);

    // 80256B88: lfs     f1, 8(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(8));

    // 80256B8C: lfs     f0, 28(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(28));

    // 80256B90: fmuls   f2, f1, f0
    ppc_fmuls(cpu, 2, 1, 0);

    // 80256B94: lfs     f1, 12(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(12));

    // 80256B98: lfs     f0, 32(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(32));

    // 80256B9C: fmuls   f0, f1, f0
    ppc_fmuls(cpu, 0, 1, 0);

    // 80256BA0: fadds   f0, f2, f0
    ppc_fadds(cpu, 0, 2, 0);

    // 80256BA4: fadds   f0, f3, f0
    ppc_fadds(cpu, 0, 3, 0);

    // 80256BA8: fcmpo   cr0, f0, f31
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[31], true);

    // 80256BAC: bc    4, 1, 0x80256BC0
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_80256BC0;

    cpu->downcount -= 4u;
    // 80256BB0: lwz     r3, 28(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(28);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 80256BB4: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 80256BB8: stw     r0, 28(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(28);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 80256BBC: addi    r4, r4, 1
    cpu->gpr[4] = cpu->gpr[4] + (u32)(s32)(1);

label_80256BC0:

    cpu->downcount -= 13u;
    // 80256BC0: lfs     f1, 16(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(16));

    // 80256BC4: lfs     f0, 48(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(48));

    // 80256BC8: fmuls   f3, f1, f0
    ppc_fmuls(cpu, 3, 1, 0);

    // 80256BCC: lfs     f1, 8(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(8));

    // 80256BD0: lfs     f0, 40(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(40));

    // 80256BD4: fmuls   f2, f1, f0
    ppc_fmuls(cpu, 2, 1, 0);

    // 80256BD8: lfs     f1, 12(r1)
    lfs(cpu, 1, cpu->gpr[1] + (u32)(s32)(12));

    // 80256BDC: lfs     f0, 44(r30)
    lfs(cpu, 0, cpu->gpr[30] + (u32)(s32)(44));

    // 80256BE0: fmuls   f0, f1, f0
    ppc_fmuls(cpu, 0, 1, 0);

    // 80256BE4: fadds   f0, f2, f0
    ppc_fadds(cpu, 0, 2, 0);

    // 80256BE8: fadds   f0, f3, f0
    ppc_fadds(cpu, 0, 3, 0);

    // 80256BEC: fcmpo   cr0, f0, f31
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[31], true);

    // 80256BF0: bc    4, 1, 0x80256C04
    if ((((cpu->cr & 0x40000000u) != 0) == false))
        goto label_80256C04;

    cpu->downcount -= 4u;
    // 80256BF4: lwz     r3, 32(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(32);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 80256BF8: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 80256BFC: stw     r0, 32(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(32);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 80256C00: addi    r4, r4, 1
    cpu->gpr[4] = cpu->gpr[4] + (u32)(s32)(1);

label_80256C04:

    cpu->downcount -= 2u;
    // 80256C04: cmpwi   r4, 0
    {
        s32 val_a = (s32)(cpu->gpr[4]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 80256C08: bc    4, 2, 0x80256C14
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_80256C14;

    cpu->downcount -= 2u;
    // 80256C0C: li      r3, 0
    cpu->gpr[3] = (u32)(s32)(0);

    // 80256C10: b       0x80256C98
    {
        goto label_80256C98;
    }

label_80256C14:

    cpu->downcount -= 4u;
    // 80256C14: addi    r28, r28, 1
    cpu->gpr[28] = cpu->gpr[28] + (u32)(s32)(1);

    // 80256C18: cmplwi  r28, 0x0008
    {
        u32 val_a = (u32)(cpu->gpr[28]);
        u32 val_b = (u32)(0x0008u);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 80256C1C: addi    r29, r29, 12
    cpu->gpr[29] = cpu->gpr[29] + (u32)(s32)(12);

    // 80256C20: bc    12, 0, 0x80256A98
    {
        bool ctr_ok = true;
        bool cr_ok = (((cpu->cr & 0x80000000u) != 0) == true);
        if (ctr_ok && cr_ok) {

            goto label_80256A98;
        }
    }

    cpu->downcount -= 3u;
    // 80256C24: lwz     r0, 20(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(20);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 80256C28: cmpwi   r0, 8
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(8);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 80256C2C: bc    4, 2, 0x80256C38
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_80256C38;

    cpu->downcount -= 2u;
    // 80256C30: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 80256C34: b       0x80256C98
    {
        goto label_80256C98;
    }

label_80256C38:

    cpu->downcount -= 3u;
    // 80256C38: lwz     r0, 28(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(28);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 80256C3C: cmpwi   r0, 8
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(8);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 80256C40: bc    4, 2, 0x80256C4C
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_80256C4C;

    cpu->downcount -= 2u;
    // 80256C44: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 80256C48: b       0x80256C98
    {
        goto label_80256C98;
    }

label_80256C4C:

    cpu->downcount -= 3u;
    // 80256C4C: lwz     r0, 24(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(24);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 80256C50: cmpwi   r0, 8
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(8);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 80256C54: bc    4, 2, 0x80256C60
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_80256C60;

    cpu->downcount -= 2u;
    // 80256C58: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 80256C5C: b       0x80256C98
    {
        goto label_80256C98;
    }

label_80256C60:

    cpu->downcount -= 3u;
    // 80256C60: lwz     r0, 32(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(32);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 80256C64: cmpwi   r0, 8
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(8);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 80256C68: bc    4, 2, 0x80256C74
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_80256C74;

    cpu->downcount -= 2u;
    // 80256C6C: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 80256C70: b       0x80256C98
    {
        goto label_80256C98;
    }

label_80256C74:

    cpu->downcount -= 3u;
    // 80256C74: lwz     r0, 36(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(36);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 80256C78: cmpwi   r0, 8
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(8);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 80256C7C: bc    4, 2, 0x80256C88
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_80256C88;

    cpu->downcount -= 2u;
    // 80256C80: li      r3, 1
    cpu->gpr[3] = (u32)(s32)(1);

    // 80256C84: b       0x80256C98
    {
        goto label_80256C98;
    }

label_80256C88:

    cpu->downcount -= 4u;
    // 80256C88: lwz     r0, 40(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(40);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 80256C8C: subfic  r0, r0, 8
    {
        u64 res = (u64)(u32)(s32)(8) + (u64)(~cpu->gpr[0]) + 1u;
        cpu->gpr[0] = (u32)res;
        cpu->xer = (cpu->xer & ~0x20000000u) | (((u32)(res >> 32) & 1u) << 29);
    }

    // 80256C90: cntlzw r0, r0
    {
        u32 v = cpu->gpr[0];
        u32 n = 0;
        while (n < 32 && ((v & (0x80000000u >> n)) == 0))
            n++;
        cpu->gpr[0] = n;
    }

    // 80256C94: rlwinm r3, r0, 27, 5, 31
    {
        cpu->gpr[3] = gm_rotl32(cpu->gpr[0], 27u) & 0x07FFFFFFu;
    }

label_80256C98:

    cpu->downcount -= 4u;
    // 80256C98: psq_l   f31, 168(r1), 0, 0
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(168);
        gm_psq_load(cpu, 31u, ea);
    }

    // 80256C9C: lfd     f31, 160(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(160);
        cpu->fpr[31] = f64_value(gm_word64(cpu, ea));
    }

    // 80256CA0: addi    r11, r1, 160
    cpu->gpr[11] = cpu->gpr[1] + (u32)(s32)(160);

    // 80256CA4: restore r28-r31
    cpu->lr = 0x80256CA8u;
    cpu->downcount -= 5;
    cpu->gpr[28] = word(cpu, cpu->gpr[11] - 16u);
    cpu->gpr[29] = word(cpu, cpu->gpr[11] - 12u);
    cpu->gpr[30] = word(cpu, cpu->gpr[11] - 8u);
    cpu->gpr[31] = word(cpu, cpu->gpr[11] - 4u);

    cpu->downcount -= 5u;
    // 80256CA8: lwz     r0, 180(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(180);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 80256CAC: mtlr    r0
    cpu->lr = cpu->gpr[0];

    // 80256CB0: addi    r1, r1, 176
    cpu->gpr[1] = cpu->gpr[1] + (u32)(s32)(176);

    // 80256CB4: blr
    cpu->cycle_observation_suffix = 2;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}

static int gm_hermite(CPUState* cpu) {

    cpu->downcount -= 37u;
    // 802F06D8: psq_l   f0, 0(r3), 1, 5
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(0);
        gm_short_load(cpu, 0u, ea);
    }

    // 802F06DC: psq_l   f3, 0(r6), 1, 5
    {
        u32 ea = cpu->gpr[6] + (u32)(s32)(0);
        gm_short_load(cpu, 3u, ea);
    }

    // 802F06E0: psq_l   f2, 0(r4), 1, 5
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(0);
        gm_short_load(cpu, 2u, ea);
    }

    // 802F06E4: fsubs   f4, f3, f0
    ppc_fsubs(cpu, 4, 3, 0);

    // 802F06E8: psq_l   f3, 0(r7), 1, 5
    {
        u32 ea = cpu->gpr[7] + (u32)(s32)(0);
        gm_short_load(cpu, 3u, ea);
    }

    // 802F06EC: fsubs   f6, f1, f0
    ppc_fsubs(cpu, 6, 1, 0);

    // 802F06F0: psq_l   f1, 0(r8), 1, 5
    {
        u32 ea = cpu->gpr[8] + (u32)(s32)(0);
        gm_short_load(cpu, 1u, ea);
    }

    // 802F06F4: fsubs   f5, f3, f2
    ppc_fsubs(cpu, 5, 3, 2);

    // 802F06F8: fdivs   f6, f6, f4
    ppc_fdivs(cpu, 6, 6, 4);

    // 802F06FC: psq_l   f0, 0(r5), 1, 5
    {
        u32 ea = cpu->gpr[5] + (u32)(s32)(0);
        gm_short_load(cpu, 0u, ea);
    }

    // 802F0700: fmadds f1, f1, f4, f2
    {
        f64 result;
        if (ppc_fma(cpu, cpu->fpr[1], cpu->fpr[4], cpu->fpr[2], true, false, false, &result))
            cpu->fpr[1] = cpu->ps1[1] = result;
    }

    // 802F0704: fmuls   f7, f6, f6
    ppc_fmuls(cpu, 7, 6, 6);

    // 802F0708: fnmsubs f5, f4, f0, f5
    {
        f64 result;
        if (ppc_fma(cpu, cpu->fpr[4], cpu->fpr[0], cpu->fpr[5], true, true, true, &result))
            cpu->fpr[5] = cpu->ps1[5] = result;
    }

    // 802F070C: fsubs   f1, f1, f3
    ppc_fsubs(cpu, 1, 1, 3);

    // 802F0710: fsubs   f1, f1, f5
    ppc_fsubs(cpu, 1, 1, 5);

    // 802F0714: fmuls   f3, f7, f1
    ppc_fmuls(cpu, 3, 7, 1);

    // 802F0718: fmadds f1, f4, f0, f3
    {
        f64 result;
        if (ppc_fma(cpu, cpu->fpr[4], cpu->fpr[0], cpu->fpr[3], true, false, false, &result))
            cpu->fpr[1] = cpu->ps1[1] = result;
    }

    // 802F071C: fmadds f1, f1, f6, f2
    {
        f64 result;
        if (ppc_fma(cpu, cpu->fpr[1], cpu->fpr[6], cpu->fpr[2], true, false, false, &result))
            cpu->fpr[1] = cpu->ps1[1] = result;
    }

    // 802F0720: fmadds f1, f5, f7, f1
    {
        f64 result;
        if (ppc_fma(cpu, cpu->fpr[5], cpu->fpr[7], cpu->fpr[1], true, false, false, &result))
            cpu->fpr[1] = cpu->ps1[1] = result;
    }

    // 802F0724: fsubs   f1, f1, f3
    ppc_fsubs(cpu, 1, 1, 3);

    // 802F0728: blr
    cpu->cycle_observation_suffix = 2;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}

/* Binary search and signed-short Hermite interpolation. Strictly increasing
 * key times make every divisor nonzero; all data, conversion scratch and SDA
 * constants are checked before the frame is written. Quantised GQR5 loads
 * are native only for signed shorts with scale zero. */
static int key_s(CPUState* cpu) {
    const u32 frame = cpu->gpr[1] - 16u, table = cpu->gpr[3], data = cpu->gpr[4];
    const u32 magic = cpu->gpr[2] - 13160u;
    const u64 time = f64_bits(cpu->fpr[1]) & 0x7FFFFFFFFFFFFFFFull;
    if (!ready(cpu, 512, true) || ((cpu->gqr[5] >> 16) & 0x3F07u) != 7u || !ram_ok(cpu, frame, 24) ||
        !ram_ok(cpu, table, 8) || !ram_ok(cpu, magic, 8) || gm_word64(cpu, magic) != 0x4330000080000000ull ||
        (time != 0 && (time < 0x3C30000000000000ull || time >= 0x41D0000000000000ull)))
        return 0;
    const u32 count = gm_word16(cpu, table), stride = gm_word16(cpu, table + 4u) == 0 ? 6u : 8u;
    if (count == 0 || count > 256 || !ram_ok(cpu, data, count * stride) || !apart(frame, 24, table, 8) ||
        !apart(frame, 24, magic, 8) || !apart(frame, 24, data, count * stride))
        return 0;
    for (u32 i = 1; i < count; ++i)
        if ((s16)gm_word16(cpu, data + i * stride) <= (s16)gm_word16(cpu, data + (i - 1u) * stride))
            return 0;

    cpu->downcount -= 7u;
    // 802F072C: stwu     r1, -16(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(-16);
        store(cpu, ea, (u32)cpu->gpr[1]);
        cpu->gpr[1] = ea;
    }

    // 802F0730: mflr    r0
    cpu->gpr[0] = cpu->lr;

    // 802F0734: stw     r0, 20(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(20);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F0738: or   r8, r4, r4
    {
        cpu->gpr[8] = cpu->gpr[4] | cpu->gpr[4];
    }

    // 802F073C: lhz     r0, 4(r3)
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(4);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0740: cmplwi  r0, 0x0000
    {
        u32 val_a = (u32)(cpu->gpr[0]);
        u32 val_b = (u32)(0x0000u);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0744: bc    4, 2, 0x802F0848
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_802F0848;

    cpu->downcount -= 10u;
    // 802F0748: lha     r0, 0(r8)
    {
        u32 ea = cpu->gpr[8] + (u32)(s32)(0);
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F074C: lfd     f2, -13160(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-13160);
        cpu->fpr[2] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0750: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F0754: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F0758: lis     r4, 17200
    cpu->gpr[4] = ((u32)(s32)(17200) << 16);

    // 802F075C: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F0760: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0764: fsubs   f0, f0, f2
    ppc_fsubs(cpu, 0, 0, 2);

    // 802F0768: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 802F076C: bc    4, 0, 0x802F078C
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F078C;

    cpu->downcount -= 7u;
    // 802F0770: lha     r0, 2(r8)
    {
        u32 ea = cpu->gpr[8] + (u32)(s32)(2);
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F0774: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F0778: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F077C: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F0780: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0784: fsubs   f1, f0, f2
    ppc_fsubs(cpu, 1, 0, 2);

    // 802F0788: b       0x802F0944
    {
        goto label_802F0944;
    }

label_802F078C:

    cpu->downcount -= 14u;
    // 802F078C: lhz     r5, 0(r3)
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(0);
        cpu->gpr[5] = gm_word16(cpu, ea);
    }

    // 802F0790: addi    r0, r5, -1
    cpu->gpr[0] = cpu->gpr[5] + (u32)(s32)(-1);

    // 802F0794: mulli   r3, r0, 6
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[0] * (s64)(s32)6);

    // 802F0798: lhax    r0, r8, r3
    {
        u32 ea = cpu->gpr[8] + cpu->gpr[3];
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F079C: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F07A0: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F07A4: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F07A8: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F07AC: fsubs   f0, f0, f2
    ppc_fsubs(cpu, 0, 0, 2);

    // 802F07B0: fcmpo   cr0, f0, f1
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[1], true);

    // 802F07B4: cror    2, 0, 2
    {
        u32 a = (cpu->cr >> (31u - 0u)) & 1u;
        u32 b = (cpu->cr >> (31u - 2u)) & 1u;
        u32 mask = 0x80000000u >> 2;
        u32 value = (a | b) & 1u;
        cpu->cr = (cpu->cr & ~mask) | (value ? mask : 0u);
    }

    // 802F07B8: bc    4, 2, 0x802F0820
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_802F0820;

    cpu->downcount -= 8u;
    // 802F07BC: add   r3, r8, r3
    {
        u32 a = cpu->gpr[8];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F07C0: lha     r0, 2(r3)
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(2);
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F07C4: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F07C8: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F07CC: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F07D0: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F07D4: fsubs   f1, f0, f2
    ppc_fsubs(cpu, 1, 0, 2);

    // 802F07D8: b       0x802F0944
    {
        goto label_802F0944;
    }

    cpu->downcount -= 1u;
    // 802F07DC: b       0x802F0820
    {
        goto label_802F0820;
    }

label_802F07E0:

    cpu->downcount -= 14u;
    // 802F07E0: srawi r0, r5, 1
    {
        u32 sh = 1u;
        u32 value = cpu->gpr[5];
        bool ca = false;
        if (sh == 0) {
            cpu->gpr[0] = value;
        } else if (sh > 31) {
            cpu->gpr[0] = (value & 0x80000000u) ? 0xFFFFFFFFu : 0u;
            ca = (value & 0x80000000u) != 0;
        } else {
            cpu->gpr[0] = (u32)((s32)value >> sh);
            ca = (value & 0x80000000u) && ((value << (32u - sh)) != 0);
        }
        cpu->xer = (cpu->xer & ~0x20000000u) | (ca ? 0x20000000u : 0u);
    }

    // 802F07E4: addze  r6, r0
    {
        u32 a = cpu->gpr[0];
        u64 wide = (u64)a + ((cpu->xer >> 29) & 1u);
        u32 res = (u32)wide;
        cpu->gpr[6] = res;
        cpu->xer = (cpu->xer & ~0x20000000u) | (((u32)(wide >> 32) & 1u) << 29);
    }

    // 802F07E8: mulli   r3, r6, 6
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[6] * (s64)(s32)6);

    // 802F07EC: lhax    r0, r8, r3
    {
        u32 ea = cpu->gpr[8] + cpu->gpr[3];
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F07F0: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F07F4: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F07F8: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F07FC: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0800: fsubs   f0, f0, f2
    ppc_fsubs(cpu, 0, 0, 2);

    // 802F0804: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 802F0808: cror    2, 1, 2
    {
        u32 a = (cpu->cr >> (31u - 1u)) & 1u;
        u32 b = (cpu->cr >> (31u - 2u)) & 1u;
        u32 mask = 0x80000000u >> 2;
        u32 value = (a | b) & 1u;
        cpu->cr = (cpu->cr & ~mask) | (value ? mask : 0u);
    }

    // 802F080C: bc    4, 2, 0x802F081C
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_802F081C;

    cpu->downcount -= 3u;
    // 802F0810: add   r8, r8, r3
    {
        u32 a = cpu->gpr[8];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[8] = res;
    }

    // 802F0814: subf   r5, r6, r5
    {
        u32 a = ~cpu->gpr[6];
        u32 b = cpu->gpr[5];
        u32 res = a + b + 1u;
        cpu->gpr[5] = res;
    }

    // 802F0818: b       0x802F0820
    {
        goto label_802F0820;
    }

label_802F081C:

    cpu->downcount -= 1u;
    // 802F081C: or   r5, r6, r6
    {
        cpu->gpr[5] = cpu->gpr[6] | cpu->gpr[6];
    }

label_802F0820:

    cpu->downcount -= 2u;
    // 802F0820: cmpwi   r5, 1
    {
        s32 val_a = (s32)(cpu->gpr[5]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0824: bc    12, 1, 0x802F07E0
    {
        bool ctr_ok = true;
        bool cr_ok = (((cpu->cr & 0x40000000u) != 0) == true);
        if (ctr_ok && cr_ok) {

            goto label_802F07E0;
        }
    }

    cpu->downcount -= 7u;
    // 802F0828: or   r3, r8, r8
    {
        cpu->gpr[3] = cpu->gpr[8] | cpu->gpr[8];
    }

    // 802F082C: addi    r4, r8, 2
    cpu->gpr[4] = cpu->gpr[8] + (u32)(s32)(2);

    // 802F0830: addi    r5, r8, 4
    cpu->gpr[5] = cpu->gpr[8] + (u32)(s32)(4);

    // 802F0834: addi    r6, r8, 6
    cpu->gpr[6] = cpu->gpr[8] + (u32)(s32)(6);

    // 802F0838: addi    r7, r8, 8
    cpu->gpr[7] = cpu->gpr[8] + (u32)(s32)(8);

    // 802F083C: addi    r8, r8, 10
    cpu->gpr[8] = cpu->gpr[8] + (u32)(s32)(10);

    // 802F0840: J3DHermiteInterpolationS
    cpu->lr = 0x802F0844u;
    gm_hermite(cpu);

    cpu->downcount -= 1u;
    // 802F0844: b       0x802F0944
    {
        goto label_802F0944;
    }

label_802F0848:

    cpu->downcount -= 10u;
    // 802F0848: lha     r0, 0(r8)
    {
        u32 ea = cpu->gpr[8] + (u32)(s32)(0);
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F084C: lfd     f2, -13160(r2)
    {
        u32 ea = cpu->gpr[2] + (u32)(s32)(-13160);
        cpu->fpr[2] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0850: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F0854: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F0858: lis     r4, 17200
    cpu->gpr[4] = ((u32)(s32)(17200) << 16);

    // 802F085C: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F0860: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0864: fsubs   f0, f0, f2
    ppc_fsubs(cpu, 0, 0, 2);

    // 802F0868: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 802F086C: bc    4, 0, 0x802F088C
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F088C;

    cpu->downcount -= 7u;
    // 802F0870: lha     r0, 2(r8)
    {
        u32 ea = cpu->gpr[8] + (u32)(s32)(2);
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F0874: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F0878: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F087C: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F0880: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0884: fsubs   f1, f0, f2
    ppc_fsubs(cpu, 1, 0, 2);

    // 802F0888: b       0x802F0944
    {
        goto label_802F0944;
    }

label_802F088C:

    cpu->downcount -= 12u;
    // 802F088C: lhz     r5, 0(r3)
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(0);
        cpu->gpr[5] = gm_word16(cpu, ea);
    }

    // 802F0890: addi    r0, r5, -1
    cpu->gpr[0] = cpu->gpr[5] + (u32)(s32)(-1);

    // 802F0894: rlwinm r3, r0, 3, 0, 28
    {
        cpu->gpr[3] = gm_rotl32(cpu->gpr[0], 3u) & 0xFFFFFFF8u;
    }

    // 802F0898: lhax    r0, r8, r3
    {
        u32 ea = cpu->gpr[8] + cpu->gpr[3];
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F089C: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F08A0: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F08A4: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F08A8: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F08AC: fsubs   f0, f0, f2
    ppc_fsubs(cpu, 0, 0, 2);

    // 802F08B0: fcmpo   cr0, f0, f1
    ppc_fcmp(cpu, 0, cpu->fpr[0], cpu->fpr[1], true);

    // 802F08B4: cror    2, 0, 2
    {
        u32 a = (cpu->cr >> (31u - 0u)) & 1u;
        u32 b = (cpu->cr >> (31u - 2u)) & 1u;
        u32 mask = 0x80000000u >> 2;
        u32 value = (a | b) & 1u;
        cpu->cr = (cpu->cr & ~mask) | (value ? mask : 0u);
    }

    // 802F08B8: bc    4, 2, 0x802F0920
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_802F0920;

    cpu->downcount -= 8u;
    // 802F08BC: add   r3, r8, r3
    {
        u32 a = cpu->gpr[8];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F08C0: lha     r0, 2(r3)
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(2);
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F08C4: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F08C8: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F08CC: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F08D0: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F08D4: fsubs   f1, f0, f2
    ppc_fsubs(cpu, 1, 0, 2);

    // 802F08D8: b       0x802F0944
    {
        goto label_802F0944;
    }

    cpu->downcount -= 1u;
    // 802F08DC: b       0x802F0920
    {
        goto label_802F0920;
    }

label_802F08E0:

    cpu->downcount -= 12u;
    // 802F08E0: srawi r0, r5, 1
    {
        u32 sh = 1u;
        u32 value = cpu->gpr[5];
        bool ca = false;
        if (sh == 0) {
            cpu->gpr[0] = value;
        } else if (sh > 31) {
            cpu->gpr[0] = (value & 0x80000000u) ? 0xFFFFFFFFu : 0u;
            ca = (value & 0x80000000u) != 0;
        } else {
            cpu->gpr[0] = (u32)((s32)value >> sh);
            ca = (value & 0x80000000u) && ((value << (32u - sh)) != 0);
        }
        cpu->xer = (cpu->xer & ~0x20000000u) | (ca ? 0x20000000u : 0u);
    }

    // 802F08E4: addze  r6, r0
    {
        u32 a = cpu->gpr[0];
        u64 wide = (u64)a + ((cpu->xer >> 29) & 1u);
        u32 res = (u32)wide;
        cpu->gpr[6] = res;
        cpu->xer = (cpu->xer & ~0x20000000u) | (((u32)(wide >> 32) & 1u) << 29);
    }

    // 802F08E8: rlwinm r3, r6, 3, 0, 28
    {
        cpu->gpr[3] = gm_rotl32(cpu->gpr[6], 3u) & 0xFFFFFFF8u;
    }

    // 802F08EC: lhax    r0, r8, r3
    {
        u32 ea = cpu->gpr[8] + cpu->gpr[3];
        cpu->gpr[0] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F08F0: xoris   r0, r0, 0x8000
    cpu->gpr[0] = cpu->gpr[0] ^ (0x8000u << 16);

    // 802F08F4: stw     r0, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F08F8: stw     r4, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        store(cpu, ea, (u32)cpu->gpr[4]);
    }

    // 802F08FC: lfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        cpu->fpr[0] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0900: fsubs   f0, f0, f2
    ppc_fsubs(cpu, 0, 0, 2);

    // 802F0904: fcmpo   cr0, f1, f0
    ppc_fcmp(cpu, 0, cpu->fpr[1], cpu->fpr[0], true);

    // 802F0908: cror    2, 1, 2
    {
        u32 a = (cpu->cr >> (31u - 1u)) & 1u;
        u32 b = (cpu->cr >> (31u - 2u)) & 1u;
        u32 mask = 0x80000000u >> 2;
        u32 value = (a | b) & 1u;
        cpu->cr = (cpu->cr & ~mask) | (value ? mask : 0u);
    }

    // 802F090C: bc    4, 2, 0x802F091C
    if ((((cpu->cr & 0x20000000u) != 0) == false))
        goto label_802F091C;

    cpu->downcount -= 3u;
    // 802F0910: add   r8, r8, r3
    {
        u32 a = cpu->gpr[8];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[8] = res;
    }

    // 802F0914: subf   r5, r6, r5
    {
        u32 a = ~cpu->gpr[6];
        u32 b = cpu->gpr[5];
        u32 res = a + b + 1u;
        cpu->gpr[5] = res;
    }

    // 802F0918: b       0x802F0920
    {
        goto label_802F0920;
    }

label_802F091C:

    cpu->downcount -= 1u;
    // 802F091C: or   r5, r6, r6
    {
        cpu->gpr[5] = cpu->gpr[6] | cpu->gpr[6];
    }

label_802F0920:

    cpu->downcount -= 2u;
    // 802F0920: cmpwi   r5, 1
    {
        s32 val_a = (s32)(cpu->gpr[5]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0924: bc    12, 1, 0x802F08E0
    {
        bool ctr_ok = true;
        bool cr_ok = (((cpu->cr & 0x40000000u) != 0) == true);
        if (ctr_ok && cr_ok) {

            goto label_802F08E0;
        }
    }

    cpu->downcount -= 7u;
    // 802F0928: or   r3, r8, r8
    {
        cpu->gpr[3] = cpu->gpr[8] | cpu->gpr[8];
    }

    // 802F092C: addi    r4, r8, 2
    cpu->gpr[4] = cpu->gpr[8] + (u32)(s32)(2);

    // 802F0930: addi    r5, r8, 6
    cpu->gpr[5] = cpu->gpr[8] + (u32)(s32)(6);

    // 802F0934: addi    r6, r8, 8
    cpu->gpr[6] = cpu->gpr[8] + (u32)(s32)(8);

    // 802F0938: addi    r7, r8, 10
    cpu->gpr[7] = cpu->gpr[8] + (u32)(s32)(10);

    // 802F093C: addi    r8, r8, 12
    cpu->gpr[8] = cpu->gpr[8] + (u32)(s32)(12);

    // 802F0940: J3DHermiteInterpolationS
    cpu->lr = 0x802F0944u;
    gm_hermite(cpu);

label_802F0944:

    cpu->downcount -= 5u;
    // 802F0944: lwz     r0, 20(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(20);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802F0948: mtlr    r0
    cpu->lr = cpu->gpr[0];

    // 802F094C: addi    r1, r1, 16
    cpu->gpr[1] = cpu->gpr[1] + (u32)(s32)(16);

    // 802F0950: blr
    cpu->cycle_observation_suffix = 2;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}

/* J3DAnmTransformKey::calcTransform, zero/one-key channels only. Every
 * channel/table read is validated before any stack/output writes; a channel
 * requiring interpolation declines the WHOLE function, unchanged. */
static int transform_simple(CPUState* cpu) {
    const u32 frame = cpu->gpr[1] - 48u, self = cpu->gpr[3], out = cpu->gpr[5];
    const u32 constants = cpu->gpr[2] - 13176u;
    const u64 time = f64_bits(cpu->fpr[1]) & 0x7FFFFFFFFFFFFFFFull;
    if (!ready(cpu, 256, true) || (frame & 7u) || !ram_ok(cpu, frame, 56) || !ram_ok(cpu, self, 44) ||
        !ram_ok(cpu, out, 32) || !floats_ok(cpu, constants, 2) || !apart(frame, 56, self, 44) ||
        !apart(frame, 56, out, 32) || !apart(out, 32, self, 44) || !apart(frame, 56, constants, 8) ||
        !apart(out, 32, constants, 8) ||
        (time != 0 && (time < 0x3C30000000000000ull || time >= 0x41D0000000000000ull)))
        return 0;
    const u32 table = word(cpu, self + 40u) + (cpu->gpr[4] & 0xFFFFu) * 54u;
    if ((table & 1u) || !ppc_dispatch_poll_read_stable(cpu, table, 54) || !apart(frame, 56, table, 54) ||
        !apart(out, 32, table, 54))
        return 0;
    for (u32 axis = 0; axis < 3; ++axis) {
        for (u32 channel = 0; channel < 3; ++channel) {
            const u32 key = table + axis * 18u + channel * 6u;
            const u32 count = gm_word16(cpu, key);
            if (count > 1u)
                return 0;
            if (count == 1u) {
                const u32 width = channel == 1u ? 2u : 4u;
                const u32 at = word(cpu, self + 16u + channel * 4u) + gm_word16(cpu, key + 2u) * width;
                if ((at & (width - 1u)) || !ppc_dispatch_poll_read_stable(cpu, at, width) ||
                    !apart(frame, 56, at, width) || !apart(out, 32, at, width) ||
                    (width == 4u && !bounded(word(cpu, at))))
                    return 0;
            }
        }
    }

    cpu->downcount -= 7u;
    // 802F0954: stwu     r1, -48(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(-48);
        store(cpu, ea, (u32)cpu->gpr[1]);
        cpu->gpr[1] = ea;
    }

    // 802F0958: mflr    r0
    cpu->gpr[0] = cpu->lr;

    // 802F095C: stw     r0, 52(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(52);
        store(cpu, ea, (u32)cpu->gpr[0]);
    }

    // 802F0960: stfd     f31, 32(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(32);
        gm_store64(cpu, ea, f64_bits(cpu->fpr[31]));
    }

    // 802F0964: psq_st   f31, 40(r1), 0, 0
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(40);
        gm_psq_store(cpu, 31u, ea);
    }

    // 802F0968: addi    r11, r1, 32
    cpu->gpr[11] = cpu->gpr[1] + (u32)(s32)(32);

    // 802F096C: save r28-r31
    cpu->lr = 0x802F0970u;
    cpu->downcount -= 5;
    store(cpu, cpu->gpr[11] - 16u, cpu->gpr[28]);
    store(cpu, cpu->gpr[11] - 12u, cpu->gpr[29]);
    store(cpu, cpu->gpr[11] - 8u, cpu->gpr[30]);
    store(cpu, cpu->gpr[11] - 4u, cpu->gpr[31]);

    cpu->downcount -= 12u;
    // 802F0970: or   r28, r3, r3
    {
        cpu->gpr[28] = cpu->gpr[3] | cpu->gpr[3];
    }

    // 802F0974: fmr    f31, f1
    cpu->fpr[31] = cpu->fpr[1];

    // 802F0978: or   r29, r5, r5
    {
        cpu->gpr[29] = cpu->gpr[5] | cpu->gpr[5];
    }

    // 802F097C: lwz     r6, 40(r3)
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F0980: rlwinm r30, r4, 0, 16, 31
    {
        cpu->gpr[30] = gm_rotl32(cpu->gpr[4], 0u) & 0x0000FFFFu;
    }

    // 802F0984: mulli   r31, r30, 54
    cpu->gpr[31] = (u32)((s64)(s32)cpu->gpr[30] * (s64)(s32)54);

    // 802F0988: add   r3, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F098C: lhz     r0, 0(r3)
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(0);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0990: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0994: bc    12, 2, 0x802F09B4
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F09B4;

    cpu->downcount -= 1u;
    // 802F0998: bc    4, 0, 0x802F09CC
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F09CC;

    cpu->downcount -= 2u;
    // 802F099C: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F09A0: bc    4, 0, 0x802F09A8
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F09A8;

    cpu->downcount -= 1u;
    // 802F09A4: b       0x802F09CC
    {
        goto label_802F09CC;
    }

label_802F09A8:

    cpu->downcount -= 3u;
    // 802F09A8: lfs     f0, -13176(r2)
    lfs(cpu, 0, cpu->gpr[2] + (u32)(s32)(-13176));

    // 802F09AC: stfs     f0, 0(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(0));

    // 802F09B0: b       0x802F09EC
    {
        goto label_802F09EC;
    }

label_802F09B4:

    cpu->downcount -= 6u;
    // 802F09B4: lwz     r4, 16(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(16);
        cpu->gpr[4] = word(cpu, ea);
    }

    // 802F09B8: lhz     r0, 2(r3)
    {
        u32 ea = cpu->gpr[3] + (u32)(s32)(2);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F09BC: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F09C0: lfsx    f0, r4, r0
    {
        u32 ea = cpu->gpr[4] + cpu->gpr[0];
        f64 value = gm_single(word(cpu, ea));
        cpu->fpr[0] = value;
        cpu->ps1[0] = value;
    }

    // 802F09C4: stfs     f0, 0(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(0));

    // 802F09C8: b       0x802F09EC
    {
        goto label_802F09EC;
    }

label_802F09CC:

    cpu->downcount -= 7u;
    // 802F09CC: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F09D0: lwz     r5, 16(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(16);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F09D4: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F09D8: lhz     r0, 2(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(2);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F09DC: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F09E0: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F09E4: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 1u;
    // 802F09E8: stfs     f1, 0(r29)
    stfs(cpu, 1, cpu->gpr[29] + (u32)(s32)(0));

label_802F09EC:

    cpu->downcount -= 5u;
    // 802F09EC: lwz     r6, 40(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F09F0: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F09F4: lhz     r0, 18(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(18);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F09F8: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F09FC: bc    12, 2, 0x802F0A1C
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F0A1C;

    cpu->downcount -= 1u;
    // 802F0A00: bc    4, 0, 0x802F0A34
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0A34;

    cpu->downcount -= 2u;
    // 802F0A04: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0A08: bc    4, 0, 0x802F0A10
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0A10;

    cpu->downcount -= 1u;
    // 802F0A0C: b       0x802F0A34
    {
        goto label_802F0A34;
    }

label_802F0A10:

    cpu->downcount -= 3u;
    // 802F0A10: lfs     f0, -13176(r2)
    lfs(cpu, 0, cpu->gpr[2] + (u32)(s32)(-13176));

    // 802F0A14: stfs     f0, 4(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(4));

    // 802F0A18: b       0x802F0A64
    {
        goto label_802F0A64;
    }

label_802F0A1C:

    cpu->downcount -= 6u;
    // 802F0A1C: lwz     r3, 16(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(16);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0A20: lhz     r0, 20(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(20);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0A24: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0A28: lfsx    f0, r3, r0
    {
        u32 ea = cpu->gpr[3] + cpu->gpr[0];
        f64 value = gm_single(word(cpu, ea));
        cpu->fpr[0] = value;
        cpu->ps1[0] = value;
    }

    // 802F0A2C: stfs     f0, 4(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(4));

    // 802F0A30: b       0x802F0A64
    {
        goto label_802F0A64;
    }

label_802F0A34:

    cpu->downcount -= 15u;
    // 802F0A34: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F0A38: mulli   r3, r30, 3
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[30] * (s64)(s32)3);

    // 802F0A3C: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 802F0A40: mulli   r0, r0, 18
    cpu->gpr[0] = (u32)((s64)(s32)cpu->gpr[0] * (s64)(s32)18);

    // 802F0A44: add   r3, r6, r0
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F0A48: lwz     r5, 16(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(16);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F0A4C: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0A50: lhz     r0, 20(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(20);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0A54: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0A58: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0A5C: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 1u;
    // 802F0A60: stfs     f1, 4(r29)
    stfs(cpu, 1, cpu->gpr[29] + (u32)(s32)(4));

label_802F0A64:

    cpu->downcount -= 5u;
    // 802F0A64: lwz     r6, 40(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F0A68: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0A6C: lhz     r0, 36(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(36);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0A70: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0A74: bc    12, 2, 0x802F0A94
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F0A94;

    cpu->downcount -= 1u;
    // 802F0A78: bc    4, 0, 0x802F0AAC
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0AAC;

    cpu->downcount -= 2u;
    // 802F0A7C: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0A80: bc    4, 0, 0x802F0A88
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0A88;

    cpu->downcount -= 1u;
    // 802F0A84: b       0x802F0AAC
    {
        goto label_802F0AAC;
    }

label_802F0A88:

    cpu->downcount -= 3u;
    // 802F0A88: lfs     f0, -13176(r2)
    lfs(cpu, 0, cpu->gpr[2] + (u32)(s32)(-13176));

    // 802F0A8C: stfs     f0, 8(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(8));

    // 802F0A90: b       0x802F0ADC
    {
        goto label_802F0ADC;
    }

label_802F0A94:

    cpu->downcount -= 6u;
    // 802F0A94: lwz     r3, 16(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(16);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0A98: lhz     r0, 38(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(38);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0A9C: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0AA0: lfsx    f0, r3, r0
    {
        u32 ea = cpu->gpr[3] + cpu->gpr[0];
        f64 value = gm_single(word(cpu, ea));
        cpu->fpr[0] = value;
        cpu->ps1[0] = value;
    }

    // 802F0AA4: stfs     f0, 8(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(8));

    // 802F0AA8: b       0x802F0ADC
    {
        goto label_802F0ADC;
    }

label_802F0AAC:

    cpu->downcount -= 15u;
    // 802F0AAC: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F0AB0: mulli   r3, r30, 3
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[30] * (s64)(s32)3);

    // 802F0AB4: addi    r0, r3, 2
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(2);

    // 802F0AB8: mulli   r0, r0, 18
    cpu->gpr[0] = (u32)((s64)(s32)cpu->gpr[0] * (s64)(s32)18);

    // 802F0ABC: add   r3, r6, r0
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F0AC0: lwz     r5, 16(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(16);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F0AC4: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0AC8: lhz     r0, 38(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(38);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0ACC: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0AD0: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0AD4: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 1u;
    // 802F0AD8: stfs     f1, 8(r29)
    stfs(cpu, 1, cpu->gpr[29] + (u32)(s32)(8));

label_802F0ADC:

    cpu->downcount -= 5u;
    // 802F0ADC: lwz     r6, 40(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F0AE0: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0AE4: lhz     r0, 6(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(6);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0AE8: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0AEC: bc    12, 2, 0x802F0B0C
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F0B0C;

    cpu->downcount -= 1u;
    // 802F0AF0: bc    4, 0, 0x802F0B2C
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0B2C;

    cpu->downcount -= 2u;
    // 802F0AF4: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0AF8: bc    4, 0, 0x802F0B00
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0B00;

    cpu->downcount -= 1u;
    // 802F0AFC: b       0x802F0B2C
    {
        goto label_802F0B2C;
    }

label_802F0B00:

    cpu->downcount -= 3u;
    // 802F0B00: li      r0, 0
    cpu->gpr[0] = (u32)(s32)(0);

    // 802F0B04: sth     r0, 12(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(12);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

    // 802F0B08: b       0x802F0B68
    {
        goto label_802F0B68;
    }

label_802F0B0C:

    cpu->downcount -= 8u;
    // 802F0B0C: lwz     r3, 20(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(20);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0B10: lhz     r0, 8(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(8);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0B14: rlwinm r0, r0, 1, 0, 30
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 1u) & 0xFFFFFFFEu;
    }

    // 802F0B18: lhax    r3, r3, r0
    {
        u32 ea = cpu->gpr[3] + cpu->gpr[0];
        cpu->gpr[3] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F0B1C: lwz     r0, 36(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(36);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802F0B20: slw   r0, r3, r0
    {
        u32 sh = cpu->gpr[0] & 0x3Fu;
        cpu->gpr[0] = sh > 31 ? 0u : (cpu->gpr[3] << sh);
    }

    // 802F0B24: sth     r0, 12(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(12);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

    // 802F0B28: b       0x802F0B68
    {
        goto label_802F0B68;
    }

label_802F0B2C:

    cpu->downcount -= 9u;
    // 802F0B2C: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F0B30: addi    r3, r31, 6
    cpu->gpr[3] = cpu->gpr[31] + (u32)(s32)(6);

    // 802F0B34: add   r3, r6, r3
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F0B38: lwz     r5, 20(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(20);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F0B3C: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0B40: lhz     r0, 8(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(8);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0B44: rlwinm r0, r0, 1, 0, 30
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 1u) & 0xFFFFFFFEu;
    }

    // 802F0B48: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0B4C: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 6u;
    // 802F0B50: fctiwz    f0, f1
    {
        u64 result;
        if (ppc_fctiw(cpu, cpu->fpr[1], true, &result))
            cpu->fpr[0] = f64_value(result);
    }

    // 802F0B54: stfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        gm_store64(cpu, ea, f64_bits(cpu->fpr[0]));
    }

    // 802F0B58: lwz     r3, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0B5C: lwz     r0, 36(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(36);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802F0B60: slw   r0, r3, r0
    {
        u32 sh = cpu->gpr[0] & 0x3Fu;
        cpu->gpr[0] = sh > 31 ? 0u : (cpu->gpr[3] << sh);
    }

    // 802F0B64: sth     r0, 12(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(12);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

label_802F0B68:

    cpu->downcount -= 5u;
    // 802F0B68: lwz     r6, 40(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F0B6C: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0B70: lhz     r0, 24(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(24);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0B74: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0B78: bc    12, 2, 0x802F0B98
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F0B98;

    cpu->downcount -= 1u;
    // 802F0B7C: bc    4, 0, 0x802F0BB8
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0BB8;

    cpu->downcount -= 2u;
    // 802F0B80: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0B84: bc    4, 0, 0x802F0B8C
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0B8C;

    cpu->downcount -= 1u;
    // 802F0B88: b       0x802F0BB8
    {
        goto label_802F0BB8;
    }

label_802F0B8C:

    cpu->downcount -= 3u;
    // 802F0B8C: li      r0, 0
    cpu->gpr[0] = (u32)(s32)(0);

    // 802F0B90: sth     r0, 14(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(14);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

    // 802F0B94: b       0x802F0C00
    {
        goto label_802F0C00;
    }

label_802F0B98:

    cpu->downcount -= 8u;
    // 802F0B98: lwz     r3, 20(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(20);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0B9C: lhz     r0, 26(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(26);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0BA0: rlwinm r0, r0, 1, 0, 30
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 1u) & 0xFFFFFFFEu;
    }

    // 802F0BA4: lhax    r3, r3, r0
    {
        u32 ea = cpu->gpr[3] + cpu->gpr[0];
        cpu->gpr[3] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F0BA8: lwz     r0, 36(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(36);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802F0BAC: slw   r0, r3, r0
    {
        u32 sh = cpu->gpr[0] & 0x3Fu;
        cpu->gpr[0] = sh > 31 ? 0u : (cpu->gpr[3] << sh);
    }

    // 802F0BB0: sth     r0, 14(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(14);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

    // 802F0BB4: b       0x802F0C00
    {
        goto label_802F0C00;
    }

label_802F0BB8:

    cpu->downcount -= 16u;
    // 802F0BB8: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F0BBC: mulli   r3, r30, 3
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[30] * (s64)(s32)3);

    // 802F0BC0: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 802F0BC4: mulli   r3, r0, 18
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[0] * (s64)(s32)18);

    // 802F0BC8: addi    r3, r3, 6
    cpu->gpr[3] = cpu->gpr[3] + (u32)(s32)(6);

    // 802F0BCC: add   r3, r6, r3
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F0BD0: lwz     r5, 20(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(20);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F0BD4: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0BD8: lhz     r0, 26(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(26);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0BDC: rlwinm r0, r0, 1, 0, 30
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 1u) & 0xFFFFFFFEu;
    }

    // 802F0BE0: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0BE4: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 6u;
    // 802F0BE8: fctiwz    f0, f1
    {
        u64 result;
        if (ppc_fctiw(cpu, cpu->fpr[1], true, &result))
            cpu->fpr[0] = f64_value(result);
    }

    // 802F0BEC: stfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        gm_store64(cpu, ea, f64_bits(cpu->fpr[0]));
    }

    // 802F0BF0: lwz     r3, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0BF4: lwz     r0, 36(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(36);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802F0BF8: slw   r0, r3, r0
    {
        u32 sh = cpu->gpr[0] & 0x3Fu;
        cpu->gpr[0] = sh > 31 ? 0u : (cpu->gpr[3] << sh);
    }

    // 802F0BFC: sth     r0, 14(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(14);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

label_802F0C00:

    cpu->downcount -= 5u;
    // 802F0C00: lwz     r6, 40(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F0C04: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0C08: lhz     r0, 42(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(42);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0C0C: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0C10: bc    12, 2, 0x802F0C30
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F0C30;

    cpu->downcount -= 1u;
    // 802F0C14: bc    4, 0, 0x802F0C50
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0C50;

    cpu->downcount -= 2u;
    // 802F0C18: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0C1C: bc    4, 0, 0x802F0C24
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0C24;

    cpu->downcount -= 1u;
    // 802F0C20: b       0x802F0C50
    {
        goto label_802F0C50;
    }

label_802F0C24:

    cpu->downcount -= 3u;
    // 802F0C24: li      r0, 0
    cpu->gpr[0] = (u32)(s32)(0);

    // 802F0C28: sth     r0, 16(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(16);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

    // 802F0C2C: b       0x802F0C98
    {
        goto label_802F0C98;
    }

label_802F0C30:

    cpu->downcount -= 8u;
    // 802F0C30: lwz     r3, 20(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(20);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0C34: lhz     r0, 44(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(44);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0C38: rlwinm r0, r0, 1, 0, 30
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 1u) & 0xFFFFFFFEu;
    }

    // 802F0C3C: lhax    r3, r3, r0
    {
        u32 ea = cpu->gpr[3] + cpu->gpr[0];
        cpu->gpr[3] = (u32)(s32)(s16)gm_word16(cpu, ea);
    }

    // 802F0C40: lwz     r0, 36(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(36);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802F0C44: slw   r0, r3, r0
    {
        u32 sh = cpu->gpr[0] & 0x3Fu;
        cpu->gpr[0] = sh > 31 ? 0u : (cpu->gpr[3] << sh);
    }

    // 802F0C48: sth     r0, 16(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(16);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

    // 802F0C4C: b       0x802F0C98
    {
        goto label_802F0C98;
    }

label_802F0C50:

    cpu->downcount -= 16u;
    // 802F0C50: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F0C54: mulli   r3, r30, 3
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[30] * (s64)(s32)3);

    // 802F0C58: addi    r0, r3, 2
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(2);

    // 802F0C5C: mulli   r3, r0, 18
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[0] * (s64)(s32)18);

    // 802F0C60: addi    r3, r3, 6
    cpu->gpr[3] = cpu->gpr[3] + (u32)(s32)(6);

    // 802F0C64: add   r3, r6, r3
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F0C68: lwz     r5, 20(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(20);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F0C6C: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0C70: lhz     r0, 44(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(44);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0C74: rlwinm r0, r0, 1, 0, 30
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 1u) & 0xFFFFFFFEu;
    }

    // 802F0C78: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0C7C: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 6u;
    // 802F0C80: fctiwz    f0, f1
    {
        u64 result;
        if (ppc_fctiw(cpu, cpu->fpr[1], true, &result))
            cpu->fpr[0] = f64_value(result);
    }

    // 802F0C84: stfd     f0, 8(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(8);
        gm_store64(cpu, ea, f64_bits(cpu->fpr[0]));
    }

    // 802F0C88: lwz     r3, 12(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(12);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0C8C: lwz     r0, 36(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(36);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802F0C90: slw   r0, r3, r0
    {
        u32 sh = cpu->gpr[0] & 0x3Fu;
        cpu->gpr[0] = sh > 31 ? 0u : (cpu->gpr[3] << sh);
    }

    // 802F0C94: sth     r0, 16(r29)
    {
        u32 ea = cpu->gpr[29] + (u32)(s32)(16);
        gm_store16(cpu, ea, (u16)cpu->gpr[0]);
    }

label_802F0C98:

    cpu->downcount -= 5u;
    // 802F0C98: lwz     r6, 40(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F0C9C: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0CA0: lhz     r0, 12(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(12);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0CA4: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0CA8: bc    12, 2, 0x802F0CC8
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F0CC8;

    cpu->downcount -= 1u;
    // 802F0CAC: bc    4, 0, 0x802F0CE0
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0CE0;

    cpu->downcount -= 2u;
    // 802F0CB0: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0CB4: bc    4, 0, 0x802F0CBC
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0CBC;

    cpu->downcount -= 1u;
    // 802F0CB8: b       0x802F0CE0
    {
        goto label_802F0CE0;
    }

label_802F0CBC:

    cpu->downcount -= 3u;
    // 802F0CBC: lfs     f0, -13172(r2)
    lfs(cpu, 0, cpu->gpr[2] + (u32)(s32)(-13172));

    // 802F0CC0: stfs     f0, 20(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(20));

    // 802F0CC4: b       0x802F0D08
    {
        goto label_802F0D08;
    }

label_802F0CC8:

    cpu->downcount -= 6u;
    // 802F0CC8: lwz     r3, 24(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(24);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0CCC: lhz     r0, 14(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(14);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0CD0: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0CD4: lfsx    f0, r3, r0
    {
        u32 ea = cpu->gpr[3] + cpu->gpr[0];
        f64 value = gm_single(word(cpu, ea));
        cpu->fpr[0] = value;
        cpu->ps1[0] = value;
    }

    // 802F0CD8: stfs     f0, 20(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(20));

    // 802F0CDC: b       0x802F0D08
    {
        goto label_802F0D08;
    }

label_802F0CE0:

    cpu->downcount -= 9u;
    // 802F0CE0: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F0CE4: addi    r3, r31, 12
    cpu->gpr[3] = cpu->gpr[31] + (u32)(s32)(12);

    // 802F0CE8: add   r3, r6, r3
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F0CEC: lwz     r5, 24(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(24);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F0CF0: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0CF4: lhz     r0, 14(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(14);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0CF8: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0CFC: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0D00: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 1u;
    // 802F0D04: stfs     f1, 20(r29)
    stfs(cpu, 1, cpu->gpr[29] + (u32)(s32)(20));

label_802F0D08:

    cpu->downcount -= 5u;
    // 802F0D08: lwz     r6, 40(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F0D0C: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0D10: lhz     r0, 30(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(30);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0D14: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0D18: bc    12, 2, 0x802F0D38
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F0D38;

    cpu->downcount -= 1u;
    // 802F0D1C: bc    4, 0, 0x802F0D50
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0D50;

    cpu->downcount -= 2u;
    // 802F0D20: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0D24: bc    4, 0, 0x802F0D2C
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0D2C;

    cpu->downcount -= 1u;
    // 802F0D28: b       0x802F0D50
    {
        goto label_802F0D50;
    }

label_802F0D2C:

    cpu->downcount -= 3u;
    // 802F0D2C: lfs     f0, -13172(r2)
    lfs(cpu, 0, cpu->gpr[2] + (u32)(s32)(-13172));

    // 802F0D30: stfs     f0, 24(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(24));

    // 802F0D34: b       0x802F0D84
    {
        goto label_802F0D84;
    }

label_802F0D38:

    cpu->downcount -= 6u;
    // 802F0D38: lwz     r3, 24(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(24);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0D3C: lhz     r0, 32(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(32);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0D40: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0D44: lfsx    f0, r3, r0
    {
        u32 ea = cpu->gpr[3] + cpu->gpr[0];
        f64 value = gm_single(word(cpu, ea));
        cpu->fpr[0] = value;
        cpu->ps1[0] = value;
    }

    // 802F0D48: stfs     f0, 24(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(24));

    // 802F0D4C: b       0x802F0D84
    {
        goto label_802F0D84;
    }

label_802F0D50:

    cpu->downcount -= 16u;
    // 802F0D50: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F0D54: mulli   r3, r30, 3
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[30] * (s64)(s32)3);

    // 802F0D58: addi    r0, r3, 1
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(1);

    // 802F0D5C: mulli   r3, r0, 18
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[0] * (s64)(s32)18);

    // 802F0D60: addi    r3, r3, 12
    cpu->gpr[3] = cpu->gpr[3] + (u32)(s32)(12);

    // 802F0D64: add   r3, r6, r3
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F0D68: lwz     r5, 24(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(24);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F0D6C: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0D70: lhz     r0, 32(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(32);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0D74: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0D78: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0D7C: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 1u;
    // 802F0D80: stfs     f1, 24(r29)
    stfs(cpu, 1, cpu->gpr[29] + (u32)(s32)(24));

label_802F0D84:

    cpu->downcount -= 5u;
    // 802F0D84: lwz     r6, 40(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(40);
        cpu->gpr[6] = word(cpu, ea);
    }

    // 802F0D88: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0D8C: lhz     r0, 48(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(48);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0D90: cmpwi   r0, 1
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(1);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0D94: bc    12, 2, 0x802F0DB4
    if ((((cpu->cr & 0x20000000u) != 0) == true))
        goto label_802F0DB4;

    cpu->downcount -= 1u;
    // 802F0D98: bc    4, 0, 0x802F0DCC
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0DCC;

    cpu->downcount -= 2u;
    // 802F0D9C: cmpwi   r0, 0
    {
        s32 val_a = (s32)(cpu->gpr[0]);
        s32 val_b = (s32)(0);
        u32 cr_bits = 0;
        if (val_a < val_b)
            cr_bits |= 0x8u;
        if (val_a > val_b)
            cr_bits |= 0x4u;
        if (val_a == val_b)
            cr_bits |= 0x2u;
        cr_bits |= (cpu->xer >> 31) & 1u;
        cpu->cr = (cpu->cr & ~(0xFu << 28)) | (cr_bits << 28);
    }

    // 802F0DA0: bc    4, 0, 0x802F0DA8
    if ((((cpu->cr & 0x80000000u) != 0) == false))
        goto label_802F0DA8;

    cpu->downcount -= 1u;
    // 802F0DA4: b       0x802F0DCC
    {
        goto label_802F0DCC;
    }

label_802F0DA8:

    cpu->downcount -= 3u;
    // 802F0DA8: lfs     f0, -13172(r2)
    lfs(cpu, 0, cpu->gpr[2] + (u32)(s32)(-13172));

    // 802F0DAC: stfs     f0, 28(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(28));

    // 802F0DB0: b       0x802F0E00
    {
        goto label_802F0E00;
    }

label_802F0DB4:

    cpu->downcount -= 6u;
    // 802F0DB4: lwz     r3, 24(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(24);
        cpu->gpr[3] = word(cpu, ea);
    }

    // 802F0DB8: lhz     r0, 50(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(50);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0DBC: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0DC0: lfsx    f0, r3, r0
    {
        u32 ea = cpu->gpr[3] + cpu->gpr[0];
        f64 value = gm_single(word(cpu, ea));
        cpu->fpr[0] = value;
        cpu->ps1[0] = value;
    }

    // 802F0DC4: stfs     f0, 28(r29)
    stfs(cpu, 0, cpu->gpr[29] + (u32)(s32)(28));

    // 802F0DC8: b       0x802F0E00
    {
        goto label_802F0E00;
    }

label_802F0DCC:

    cpu->downcount -= 16u;
    // 802F0DCC: fmr    f1, f31
    cpu->fpr[1] = cpu->fpr[31];

    // 802F0DD0: mulli   r3, r30, 3
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[30] * (s64)(s32)3);

    // 802F0DD4: addi    r0, r3, 2
    cpu->gpr[0] = cpu->gpr[3] + (u32)(s32)(2);

    // 802F0DD8: mulli   r3, r0, 18
    cpu->gpr[3] = (u32)((s64)(s32)cpu->gpr[0] * (s64)(s32)18);

    // 802F0DDC: addi    r3, r3, 12
    cpu->gpr[3] = cpu->gpr[3] + (u32)(s32)(12);

    // 802F0DE0: add   r3, r6, r3
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[3];
        u32 res = a + b;
        cpu->gpr[3] = res;
    }

    // 802F0DE4: lwz     r5, 24(r28)
    {
        u32 ea = cpu->gpr[28] + (u32)(s32)(24);
        cpu->gpr[5] = word(cpu, ea);
    }

    // 802F0DE8: add   r4, r6, r31
    {
        u32 a = cpu->gpr[6];
        u32 b = cpu->gpr[31];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0DEC: lhz     r0, 50(r4)
    {
        u32 ea = cpu->gpr[4] + (u32)(s32)(50);
        cpu->gpr[0] = gm_word16(cpu, ea);
    }

    // 802F0DF0: rlwinm r0, r0, 2, 0, 29
    {
        cpu->gpr[0] = gm_rotl32(cpu->gpr[0], 2u) & 0xFFFFFFFCu;
    }

    // 802F0DF4: add   r4, r5, r0
    {
        u32 a = cpu->gpr[5];
        u32 b = cpu->gpr[0];
        u32 res = a + b;
        cpu->gpr[4] = res;
    }

    // 802F0DF8: multi-key path excluded by preflight
    __builtin_unreachable();

    cpu->downcount -= 1u;
    // 802F0DFC: stfs     f1, 28(r29)
    stfs(cpu, 1, cpu->gpr[29] + (u32)(s32)(28));

label_802F0E00:

    cpu->downcount -= 4u;
    // 802F0E00: psq_l   f31, 40(r1), 0, 0
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(40);
        gm_psq_load(cpu, 31u, ea);
    }

    // 802F0E04: lfd     f31, 32(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(32);
        cpu->fpr[31] = f64_value(gm_word64(cpu, ea));
    }

    // 802F0E08: addi    r11, r1, 32
    cpu->gpr[11] = cpu->gpr[1] + (u32)(s32)(32);

    // 802F0E0C: restore r28-r31
    cpu->lr = 0x802F0E10u;
    cpu->downcount -= 5;
    cpu->gpr[28] = word(cpu, cpu->gpr[11] - 16u);
    cpu->gpr[29] = word(cpu, cpu->gpr[11] - 12u);
    cpu->gpr[30] = word(cpu, cpu->gpr[11] - 8u);
    cpu->gpr[31] = word(cpu, cpu->gpr[11] - 4u);

    cpu->downcount -= 5u;
    // 802F0E10: lwz     r0, 52(r1)
    {
        u32 ea = cpu->gpr[1] + (u32)(s32)(52);
        cpu->gpr[0] = word(cpu, ea);
    }

    // 802F0E14: mtlr    r0
    cpu->lr = cpu->gpr[0];

    // 802F0E18: addi    r1, r1, 48
    cpu->gpr[1] = cpu->gpr[1] + (u32)(s32)(48);

    // 802F0E1C: blr
    cpu->cycle_observation_suffix = 2;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}

int bluewake_native_game_math(CPUState* cpu, u32 address) {
    unsigned kind;
    switch (address) {
    case 0x80245674u:
        kind = GM_ADD;
        break;
    case 0x802456C4u:
        kind = GM_SUB;
        break;
    case 0x80245714u:
        kind = GM_SCALE;
        break;
    case 0x8024A8E0u:
        kind = GM_CYL;
        break;
    case 0x8000CD28u:
        kind = GM_XROT;
        break;
    case 0x8000CDC8u:
        kind = GM_YROT;
        break;
    case 0x8000CE68u:
        kind = GM_ZROT;
        break;
    case 0x8024AE3Cu:
        kind = GM_BOX;
        break;
    case 0x80256888u:
        kind = GM_SPHERE_CLIP;
        break;
    case 0x802569D0u:
        kind = GM_BOX_CLIP;
        break;
    case 0x802F072Cu:
        kind = GM_KEY;
        break;
    case 0x802F0954u:
        kind = GM_TRANSFORM;
        break;
    default:
        return 0;
    }
    int ok = kind <= GM_SCALE         ? xyz(cpu, kind)
             : kind == GM_CYL         ? aab_cyl(cpu)
             : kind == GM_BOX         ? box_line(cpu)
             : kind == GM_SPHERE_CLIP ? sphere_clip(cpu)
             : kind == GM_BOX_CLIP    ? box_clip(cpu)
             : kind == GM_KEY         ? key_s(cpu)
             : kind == GM_TRANSFORM   ? transform_simple(cpu)
                                      : rot_s(cpu, kind - GM_XROT);
    if (ok)
        ++runs[kind];
    else
        ++declines[kind];
    return ok;
}
