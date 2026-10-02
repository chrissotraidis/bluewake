/* J3DModel::calcWeightEnvelopeMtx (GZLE01 0x802EE67C to 0x802EE874), native.
 *
 * A skinned model's weighted envelope matrices: for each envelope, the sum
 * over its joints of weight x (the joint's world matrix x its inverse bind
 * matrix), 3x4, in paired singles. Translated, each of its instructions
 * carries the block machinery (pc, cycle suffix, deadline refund) and each
 * paired-single operation writes two registers and the FPSCR; it was the game
 * thread's largest single function in the donor's native 60 Hz Outset
 * measurement (3.1 percent); this is not a BlueWake benchmark.
 *
 * This runs the same operations in the same order on the same values with the
 * translation's own arithmetic (inline_fp.h: the interpreter's multiplier and
 * single rounding), and leaves the machine as the translation does: the
 * registers, CR0, the FPRF, the stack frame, the matrices and flags written,
 * the reservation, the cycles (58, and per envelope 23 and 60 a joint) and
 * the last block's cycle suffix.
 *
 * It declines, changing nothing, unless that result is certain: FP available,
 * paired singles unscaled, rounding to nearest, no write journal, every
 * address plain RAM, no store overlapping anything the function reads (so the
 * order of its loads and stores cannot matter), every float it reads finite
 * and below 2^31 in magnitude (so no product, sum or single rounding reaches
 * infinity or NaN, and every paired-single operation takes the inline path
 * the translation takes), and the whole function inside the turn's cycle
 * budget and before the next deadline (so the translation would neither have
 * stopped part of the way through nor charged a block instruction by
 * instruction).
 *
 * scripts/windows/native_skin.py routes the certified function entry once
 * the translated body is the one this was checked against
 * (tests/native_skin_test.c, every register and byte against the translation).
 * No identifier here may be `ctx`. */
#include "native_skin.h"
#include "inline_fp.h"

#include <stdio.h>

#if defined(_WIN32)
#define BW_SKIN_EXPORT __declspec(dllexport)
#else
#define BW_SKIN_EXPORT __attribute__((visibility("default")))
#endif
static BluewakeNativeSkinReady s_ready;
static void* s_ready_user;
BW_SKIN_EXPORT int bluewake_composite_native_skin_v1(
    bool enabled, BluewakeNativeSkinReady ready, void* user) {
    s_ready = enabled ? ready : NULL;
    s_ready_user = s_ready != NULL ? user : NULL;
    return s_ready != NULL;
}
int bluewake_native_skin_try(CPUState* cpu) {
    return cpu != NULL && s_ready != NULL &&
           s_ready(s_ready_user, cpu, BLUEWAKE_NATIVE_SKIN_ENTRY) && bluewake_native_skin(cpu);
}
static unsigned long long s_skin_runs, s_skin_declined, s_skin_joints;

BW_SKIN_EXPORT void bluewake_native_skin_report(void) {
    fprintf(stderr, "[native-skin] calcWeightEnvelopeMtx native=%llu declined=%llu joints=%llu\n",
            s_skin_runs, s_skin_declined, s_skin_joints);
}

#define SKIN_FRAME 112u
#define SKIN_UNIT_OFFSET (-31288)   /* J3DUnit01, from r13 */
#define SKIN_SAVE_RETURN 0x802EE6B8u  /* the bl to _savegpr_29 */
#define SKIN_REST_RETURN 0x802EE864u  /* the bl to _restgpr_29 */

typedef struct SkinPair { f64 p0, p1; } SkinPair;

static inline const u8* skin_ram(const CPUState* cpu, u32 address, u32 size) {
    if (cpu->ram == NULL || !ppc_dispatch_poll_read_stable((CPUState*)cpu, address, size))
        return NULL;
    return cpu->ram + (address - GC_RAM_BASE);
}

static inline bool skin_apart(u32 a, u32 a_size, u32 b, u32 b_size) {
    return a_size == 0u || b_size == 0u ||
           (u64)a + a_size <= (u64)b || (u64)b + b_size <= (u64)a;
}

/* Finite, and below 2^31 in magnitude (denormals and zeros included). */
static inline bool skin_float(u32 bits) {
    return (bits & 0x7F800000u) <= (157u << 23);
}

static inline bool skin_floats(const u8* p, unsigned count) {
    bool ok = true;
    for (unsigned i = 0; i < count; ++i)
        ok &= skin_float(read_be32(p + 4u * i));
    return ok;
}

/* The three regions the function stores to, which nothing it reads may touch. */
typedef struct SkinStores { u32 frame, matrices, matrices_size, flags, flags_size; } SkinStores;

static inline bool skin_untouched(const SkinStores* s, u32 address, u32 size) {
    return skin_apart(address, size, s->frame, SKIN_FRAME + 8u) &&
           skin_apart(address, size, s->matrices, s->matrices_size) &&
           skin_apart(address, size, s->flags, s->flags_size);
}

static inline f64 skin_single_value(u32 bits) {
    return f64_value(convert_to_double(bits));
}

static inline SkinPair skin_psq_l(const CPUState* cpu, u32 address) {
    const u8* p = cpu->ram + (address - GC_RAM_BASE);
    return (SkinPair){skin_single_value(read_be32(p)), skin_single_value(read_be32(p + 4u))};
}

static inline void skin_store32(CPUState* cpu, u32 address, u32 value) {
    clear_matching_reservation(cpu, address);
    write_be32(cpu->ram + (address - GC_RAM_BASE), value);
}

static inline void skin_psq_st(CPUState* cpu, u32 address, SkinPair value) {
    skin_store32(cpu, address, convert_to_single_ftz(f64_bits(value.p0)));
    skin_store32(cpu, address + 4u, convert_to_single_ftz(f64_bits(value.p1)));
}

/* bw_fp_ps_muls0/1, bw_fp_ps_madds0/1 and bw_fp_ps_madd_op on finite operands,
 * as values: the FPRF is the class of the first half. */
static inline SkinPair skin_pair(const CPUState* cpu, f64 r0, f64 r1, u32* fprf) {
    const f32 s0 = bw_fp_single(cpu, r0), s1 = bw_fp_single(cpu, r1);
    *fprf = bw_fp_class32(s0);
    return (SkinPair){(f64)s0, (f64)s1};
}

static inline SkinPair skin_muls(const CPUState* cpu, SkinPair a, f64 c, u32* fprf) {
    const f64 c_round = bw_fp_25bit(c);
    return skin_pair(cpu, a.p0 * c_round, a.p1 * c_round, fprf);
}

static inline SkinPair skin_madds(const CPUState* cpu, SkinPair a, f64 c, SkinPair b, u32* fprf) {
    const f64 c_round = bw_fp_25bit(c);
    return skin_pair(cpu, bw_fp_fma_single(a.p0, c_round, b.p0), bw_fp_fma_single(a.p1, c_round, b.p1), fprf);
}

static inline SkinPair skin_madd(const CPUState* cpu, SkinPair a, SkinPair c, SkinPair b, u32* fprf) {
    return skin_pair(cpu, bw_fp_fma_single(a.p0, bw_fp_25bit(c.p0), b.p0),
                     bw_fp_fma_single(a.p1, bw_fp_25bit(c.p1), b.p1), fprf);
}

int bluewake_native_skin(CPUState* cpu) {
    if (cpu == NULL)
        return 0;
    const u32 gqr = cpu->gqr[0];
    if (cpu->exception != 0u || (cpu->msr & PPC_MSR_FP) == 0u || (cpu->hid2 & PPC_HID2_LSQE) == 0u ||
        ((gqr >> 16) & 7u) != 0u || (gqr & 7u) != 0u || (cpu->fpscr & 3u) != 0u ||
        g_mem_write_journal != NULL || cpu->cycle_budget <= 0)
        goto decline;

    const u32 self = cpu->gpr[3];
    const u32 sp = cpu->gpr[1];
    const u32 frame = sp - SKIN_FRAME;
    const u32 unit = cpu->gpr[13] + (u32)(s32)SKIN_UNIT_OFFSET;
    const u8* self_model = skin_ram(cpu, self + 4u, 4u);
    const u8* self_arrays = skin_ram(cpu, self + 132u, 16u);
    const u8* unit_data = skin_ram(cpu, unit, 8u);
    if (self_model == NULL || self_arrays == NULL || unit_data == NULL || !skin_floats(unit_data, 2) ||
        skin_ram(cpu, frame, SKIN_FRAME + 8u) == NULL)
        goto decline;
    const u32 model = read_be32(self_model);
    const u32 scale_flags = read_be32(self_arrays);        /* 132(r3) */
    const u32 envelope_flags = read_be32(self_arrays + 4); /* 136(r3) */
    const u32 node_matrices = read_be32(self_arrays + 8);  /* 140(r3) */
    const u32 weighted = read_be32(self_arrays + 12);      /* 144(r3) */
    const u8* model_fields = skin_ram(cpu, model + 48u, 20u);
    if (model_fields == NULL)
        goto decline;
    const u32 count = read_be16(model_fields);             /* 48: envelopes */
    const u32 mix_counts = read_be32(model_fields + 4);    /* 52 */
    const u32 indices = read_be32(model_fields + 8);       /* 56 */
    const u32 weights = read_be32(model_fields + 12);      /* 60 */
    const u32 inverse = read_be32(model_fields + 16);      /* 64 */

    const SkinStores stores = {frame, weighted, 48u * count, envelope_flags, count};
    if (!skin_apart(frame, SKIN_FRAME + 8u, weighted, 48u * count) ||
        !skin_apart(frame, SKIN_FRAME + 8u, envelope_flags, count) ||
        !skin_apart(weighted, 48u * count, envelope_flags, count) ||
        (count != 0u && (skin_ram(cpu, weighted, 48u * count) == NULL ||
                         skin_ram(cpu, envelope_flags, count) == NULL)) ||
        !skin_untouched(&stores, self + 4u, 4u) || !skin_untouched(&stores, self + 132u, 16u) ||
        !skin_untouched(&stores, unit, 8u) || !skin_untouched(&stores, model + 48u, 20u))
        goto decline;

    /* Every joint the function reads, checked before anything is written. */
    u64 joints = 0;
    u64 cycles = 58;
    if (count != 0u) {
        const u8* counts = skin_ram(cpu, mix_counts, count);
        if (counts == NULL || !skin_untouched(&stores, mix_counts, count))
            goto decline;
        for (u32 i = 0; i < count; ++i) {
            const u32 mix = counts[i];
            const u32 iterations = mix != 0u ? mix : 1u; /* a do-while */
            joints += iterations;
            cycles += 23u + 60u * (u64)iterations;
        }
        if (joints > 0x10000000u)
            goto decline;
        const u32 index_bytes = 2u * (u32)joints, weight_bytes = 4u * (u32)joints;
        const u8* index_data = skin_ram(cpu, indices, index_bytes);
        const u8* weight_data = skin_ram(cpu, weights, weight_bytes);
        if (index_data == NULL || weight_data == NULL || !skin_untouched(&stores, indices, index_bytes) ||
            !skin_untouched(&stores, weights, weight_bytes) || !skin_floats(weight_data, (unsigned)joints))
            goto decline;
        for (u64 j = 0; j < joints; ++j) {
            const u32 index = read_be16(index_data + 2u * j);
            const u32 a = inverse + index * 48u, b = node_matrices + index * 48u, s = scale_flags + index;
            const u8* a_data = skin_ram(cpu, a, 48u);
            const u8* b_data = skin_ram(cpu, b, 48u);
            if (a_data == NULL || b_data == NULL || skin_ram(cpu, s, 1u) == NULL ||
                !skin_floats(a_data, 12) || !skin_floats(b_data, 12) || !skin_untouched(&stores, a, 48u) ||
                !skin_untouched(&stores, b, 48u) || !skin_untouched(&stores, s, 1u))
                goto decline;
        }
    }

    /* The translation stops at no block (the turn's budget) and prepays every
     * block with no refund (the deadline, further than any block's suffix). */
    if (cpu->downcount - (s64)cycles <= -cpu->cycle_budget)
        goto decline;
    if (cpu->cycle_deadline_budget > 0 &&
        (cpu->cycle_deadline_budget < 60 || cpu->cycle_deadline_budget + cpu->downcount < (s64)cycles))
        goto decline;

    /* The prologue: the frame, the return address, f27-f31 twice, r29-r31. */
    const u32 return_address = cpu->lr;
    skin_store32(cpu, frame, sp);
    skin_store32(cpu, frame + 116u, return_address);
    for (unsigned k = 0; k < 5; ++k) {
        const unsigned r = 31u - k;
        const u32 at = frame + 96u - 16u * k;
        clear_matching_reservation(cpu, at);
        write_be64(cpu->ram + (at - GC_RAM_BASE), f64_bits(cpu->fpr[r]));
        skin_psq_st(cpu, at + 8u, (SkinPair){cpu->fpr[r], cpu->ps1[r]});
    }
    skin_store32(cpu, frame + 20u, cpu->gpr[29]);
    skin_store32(cpu, frame + 24u, cpu->gpr[30]);
    skin_store32(cpu, frame + 28u, cpu->gpr[31]);

    const SkinPair unit_pair = skin_psq_l(cpu, unit);
    const SkinPair zero = {unit_pair.p0, unit_pair.p0}; /* ps_merge00 of f27 */
    SkinPair f[14];
    for (unsigned r = 0; r < 14; ++r)
        f[r] = (SkinPair){cpu->fpr[r], cpu->ps1[r]};
    SkinPair f28 = {0, 0}, f29 = {0, 0}, f30 = {0, 0}, f31 = zero;
    f[10] = zero;
    f[12] = zero;
    u32 fprf = (cpu->fpscr >> 12) & 0x1Fu;
    bool any_fp = false;

    u32 r5 = weights, r6 = model, r7 = unit, r9 = cpu->gpr[9], r10 = cpu->gpr[10], r12 = cpu->gpr[12];
    u32 index_at = indices, weight_at = weights;
    for (u32 i = 0; i < count; ++i) {
        r7 = envelope_flags;
        cpu->ram[envelope_flags + i - GC_RAM_BASE] = 1u;
        clear_matching_reservation(cpu, envelope_flags + i);
        r10 = weighted + 48u * i;
        f[9] = f[11] = f[13] = zero;
        r9 = cpu->ram[mix_counts + (i & 0xFFFFu) - GC_RAM_BASE];
        const u32 iterations = r9 != 0u ? r9 : 1u;
        for (r12 = 0; r12 < iterations;) {
            const u32 index = read_be16(cpu->ram + (index_at - GC_RAM_BASE));
            index_at += 2u;
            const u32 a = inverse + index * 48u, b = node_matrices + index * 48u;
            f[2] = skin_psq_l(cpu, a);
            f[1] = skin_psq_l(cpu, b);
            f[3] = skin_psq_l(cpu, b + 16u);
            f[5] = skin_psq_l(cpu, b + 32u);
            f[8] = skin_muls(cpu, f[2], f[1].p0, &fprf);
            f[6] = skin_psq_l(cpu, a + 16u);
            f30 = skin_muls(cpu, f[2], f[3].p0, &fprf);
            f29 = skin_muls(cpu, f[2], f[5].p0, &fprf);
            f[7] = skin_psq_l(cpu, a + 32u);
            f[8] = skin_madds(cpu, f[6], f[1].p1, f[8], &fprf);
            f[2] = skin_psq_l(cpu, b + 8u);
            f30 = skin_madds(cpu, f[6], f[3].p1, f30, &fprf);
            f[4] = skin_psq_l(cpu, b + 24u);
            f29 = skin_madds(cpu, f[6], f[5].p1, f29, &fprf);
            f[6] = skin_psq_l(cpu, b + 40u);
            f[8] = skin_madds(cpu, f[7], f[2].p0, f[8], &fprf);
            const f64 weight = skin_single_value(read_be32(cpu->ram + (weight_at - GC_RAM_BASE)));
            weight_at += 4u;
            f[0] = (SkinPair){weight, weight};
            f30 = skin_madds(cpu, f[7], f[4].p0, f30, &fprf);
            f29 = skin_madds(cpu, f[7], f[6].p0, f29, &fprf);
            f[7] = skin_psq_l(cpu, a + 8u);
            f[9] = skin_madds(cpu, f[8], f[0].p0, f[9], &fprf);
            f[11] = skin_madds(cpu, f30, f[0].p0, f[11], &fprf);
            f[13] = skin_madds(cpu, f29, f[0].p0, f[13], &fprf);
            f[8] = skin_psq_l(cpu, a + 24u);
            f30 = skin_muls(cpu, f[7], f[1].p0, &fprf);
            f29 = skin_muls(cpu, f[7], f[3].p0, &fprf);
            f28 = skin_muls(cpu, f[7], f[5].p0, &fprf);
            f[7] = skin_psq_l(cpu, a + 40u);
            skin_psq_st(cpu, r10, f[9]);
            f30 = skin_madds(cpu, f[8], f[1].p1, f30, &fprf);
            f29 = skin_madds(cpu, f[8], f[3].p1, f29, &fprf);
            f28 = skin_madds(cpu, f[8], f[5].p1, f28, &fprf);
            f30 = skin_madds(cpu, f[7], f[2].p0, f30, &fprf);
            f29 = skin_madds(cpu, f[7], f[4].p0, f29, &fprf);
            f28 = skin_madds(cpu, f[7], f[6].p0, f28, &fprf);
            skin_psq_st(cpu, r10 + 16u, f[11]);
            skin_psq_st(cpu, r10 + 32u, f[13]);
            f30 = skin_madd(cpu, unit_pair, f[2], f30, &fprf);
            f29 = skin_madd(cpu, unit_pair, f[4], f29, &fprf);
            f28 = skin_madd(cpu, unit_pair, f[6], f28, &fprf);
            f[10] = skin_madds(cpu, f30, f[0].p0, f[10], &fprf);
            f[12] = skin_madds(cpu, f29, f[0].p0, f[12], &fprf);
            f31 = skin_madds(cpu, f28, f[0].p0, f31, &fprf);
            any_fp = true;
            r6 = cpu->ram[envelope_flags + i - GC_RAM_BASE];
            r5 = scale_flags;
            const u8 flag = (u8)(r6 & cpu->ram[scale_flags + index - GC_RAM_BASE]);
            clear_matching_reservation(cpu, envelope_flags + i);
            cpu->ram[envelope_flags + i - GC_RAM_BASE] = flag;
            ++r12;
        }
        skin_psq_st(cpu, r10 + 8u, f[10]);
        f[10] = zero;
        skin_psq_st(cpu, r10 + 24u, f[12]);
        f[12] = zero;
        skin_psq_st(cpu, r10 + 40u, f31);
        f31 = zero;
    }

    /* The epilogue: f27-f31 back (the second halves through a single), r29-r31,
     * LR from the frame, and the return. */
    for (unsigned r = 0; r < 14; ++r) {
        cpu->fpr[r] = f[r].p0;
        cpu->ps1[r] = f[r].p1;
    }
    for (unsigned r = 27; r < 32; ++r)
        cpu->ps1[r] = skin_single_value(convert_to_single_ftz(f64_bits(cpu->ps1[r])));
    if (any_fp)
        cpu->fpscr = (cpu->fpscr & ~(0x1Fu << 12)) | (fprf << 12);
    cpu->gpr[0] = return_address;
    cpu->gpr[4] = 48u * count;
    cpu->gpr[5] = r5;
    cpu->gpr[6] = count != 0u ? r6 : model;
    cpu->gpr[7] = count != 0u ? r7 : unit;
    cpu->gpr[8] = count;
    cpu->gpr[9] = r9;
    cpu->gpr[10] = r10;
    cpu->gpr[11] = frame + 32u;
    cpu->gpr[12] = r12;
    /* cmpw r31, r8 at the loop's exit: equal, with SO from XER. */
    cpu->cr = (cpu->cr & 0x0FFFFFFFu) | ((0x2u | (cpu->xer >> 31)) << 28);
    cpu->lr = return_address;
    cpu->downcount -= (s64)cycles;
    cpu->cycle_observation_suffix = 2u;
    cpu->pc = return_address & ~3u;
    s_skin_runs++;
    s_skin_joints += joints;
    return 1;

decline:
    s_skin_declined++;
    return 0;
}
