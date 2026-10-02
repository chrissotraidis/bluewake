/* Native J3DGetTranslateRotateMtx, adapted from zeldaret/tww (CC0-1.0),
 * src/JSystem/J3DGraphBase/J3DTransform.cpp, revision 09de0609.
 *
 * Compile the recovered rotation/translation formulas as ordinary C. Keep
 * guest pointers at the boundary, and reproduce the original scratch
 * registers, single rounding, FPSCR, stores and cycle accounting. The exact
 * GZLE01 translations are certified by prepare_native_j3d.py; unsupported
 * memory, inputs and observable deadlines keep their translated bodies.
 */
#include "native_j3d.h"
#include "inline_fp.h"

#include <stdio.h>

#if defined(_WIN32)
#define BW_J3D_EXPORT __declspec(dllexport)
#else
#define BW_J3D_EXPORT __attribute__((visibility("default")))
#endif
static BluewakeNativeJ3DReady s_ready;
static void* s_ready_user;

BW_J3D_EXPORT int bluewake_composite_native_j3d_v1(
    bool enabled, BluewakeNativeJ3DReady ready, void* user) {
    s_ready = enabled ? ready : NULL;
    s_ready_user = s_ready != NULL ? user : NULL;
    return s_ready != NULL;
}

int bluewake_native_j3d_try(CPUState* cpu, u32 address) {
    return cpu != NULL && s_ready != NULL && s_ready(s_ready_user, cpu, address) &&
           bluewake_native_j3d_transform(cpu, address);
}
static unsigned long long runs[2], declined[2];

BW_J3D_EXPORT void bluewake_native_j3d_report(void) {
    fprintf(stderr, "[native-j3d] transform-info=%llu/%llu transform-angles=%llu/%llu (native/declined)\n",
            runs[0], declined[0], runs[1], declined[1]);
}

static const u8* plain_ram(const CPUState* cpu, u32 address, u32 size) {
    if (cpu->ram == NULL ||
        !ppc_dispatch_poll_read_stable((CPUState*)cpu, address, size))
        return NULL;
    return cpu->ram + (address - GC_RAM_BASE);
}

static bool apart(u32 a, u32 a_size, u32 b, u32 b_size) {
    return (u64)a + a_size <= b || (u64)b + b_size <= a;
}

static f32 single_value(u32 bits) {
    return (f32)f64_value(convert_to_double(bits));
}

/* The real sine/cosine tables contain [-1,1]. This wider finite bound also
 * admits custom tables and denormals without overflowing the formulas. */
static bool bounded(u32 bits) {
    return (bits & 0x7FFFFFFFu) <= 0x44800000u; /* 2^10 */
}

static bool trig_value(const CPUState* cpu, u32 table, u32 offset, f32* value) {
    const u8* p = plain_ram(cpu, table + offset, 4);
    if (p == NULL)
        return false;
    const u32 bits = read_be32(p);
    if (!bounded(bits))
        return false;
    *value = single_value(bits);
    return true;
}

static u32 angle_offset(u32 angle, u32 shift) {
    shift &= 63u; /* sraw uses the low six bits, after zero-extending the angle. */
    return shift >= 32u ? 0u : ((angle & 0xFFFFu) >> shift) * 4u;
}

static void set_single(CPUState* cpu, unsigned index, f32 value) {
    cpu->fpr[index] = cpu->ps1[index] = (f64)value;
}

static void store_bits(CPUState* cpu, u32 address, u32 bits) {
    clear_matching_reservation(cpu, address);
    write_be32(cpu->ram + (address - GC_RAM_BASE), bits);
}

static void store_single(CPUState* cpu, u32 address, f32 value) {
    u32 bits;
    memcpy(&bits, &value, sizeof bits);
    store_bits(cpu, address, bits);
}

/* Retain each fmuls/fadds/fsubs rounding boundary; contraction is disabled
 * for this target. Double intermediates also reproduce NI's flushing before
 * conversion to single, rather than flushing an already rounded value. */
static f32 multiply(const CPUState* cpu, f32 a, f32 b) {
    return bw_fp_single(cpu, (f64)a * (f64)b);
}

static f32 add(const CPUState* cpu, f32 a, f32 b) {
    return bw_fp_single(cpu, (f64)a + (f64)b);
}

static f32 subtract(const CPUState* cpu, f32 a, f32 b) {
    return bw_fp_single(cpu, (f64)a - (f64)b);
}

int bluewake_native_j3d_transform(CPUState* cpu, u32 address) {
    const bool info = address == BLUEWAKE_J3D_TRANSFORM_INFO;
    if (!info && address != BLUEWAKE_J3D_TRANSFORM_ANGLES)
        return 0;
    const unsigned which = info ? 0 : 1;
    const unsigned cycles = info ? 54 : 48;
    if (cpu == NULL || cpu->exception != 0 || !(cpu->msr & PPC_MSR_FP) ||
        (cpu->fpscr & 3u) != 0 || g_mem_write_journal != NULL ||
        cpu->cycle_budget <= 0 || cpu->downcount <= -cpu->cycle_budget ||
        (cpu->cycle_deadline_budget > 0 && cpu->cycle_deadline_budget + cpu->downcount < cycles))
        goto decline;

    const u32 input = cpu->gpr[3];
    const u32 output = cpu->gpr[info ? 4 : 6];
    const u32 globals = cpu->gpr[13] + (u32)(s32)-26460;
    const u8* tables = plain_ram(cpu, globals, 12);
    const u8* transform = info ? plain_ram(cpu, input, 32) : NULL;
    if (tables == NULL || plain_ram(cpu, output, 48) == NULL ||
        (info && (transform == NULL || !apart(input, 32, output, 48))))
        goto decline;

    const u32 shift = read_be32(tables);
    const u32 sin_table = read_be32(tables + 4);
    const u32 cos_table = read_be32(tables + 8);
    const u32 x = angle_offset(info ? read_be16(transform + 12) : input, shift);
    const u32 y = angle_offset(info ? read_be16(transform + 14) : cpu->gpr[4], shift);
    const u32 z = angle_offset(info ? read_be16(transform + 16) : cpu->gpr[5], shift);
    f32 sx, cx, sy, cy, sz, cz;
    if (!trig_value(cpu, sin_table, x, &sx) || !trig_value(cpu, cos_table, x, &cx) ||
        !trig_value(cpu, sin_table, y, &sy) || !trig_value(cpu, cos_table, y, &cy) ||
        !trig_value(cpu, sin_table, z, &sz) || !trig_value(cpu, cos_table, z, &cz))
        goto decline;

    /* All guards precede mutations. These are the recovered source formulas,
     * in the original store order, including the non-fused multiply/subtracts. */
    store_single(cpu, output + 32, -sy);
    store_single(cpu, output, multiply(cpu, cz, cy));
    store_single(cpu, output + 16, multiply(cpu, sz, cy));
    store_single(cpu, output + 36, multiply(cpu, cy, sx));
    store_single(cpu, output + 40, multiply(cpu, cy, cx));

    const f32 cxsz = multiply(cpu, cx, sz);
    const f32 sxcz = multiply(cpu, sx, cz);
    store_single(cpu, output + 4, subtract(cpu, multiply(cpu, sxcz, sy), cxsz));
    store_single(cpu, output + 24, subtract(cpu, multiply(cpu, cxsz, sy), sxcz));

    const f32 sxsz = multiply(cpu, sx, sz);
    const f32 cxcz = multiply(cpu, cx, cz);
    store_single(cpu, output + 8, add(cpu, sxsz, multiply(cpu, cxcz, sy)));
    const f32 yy = add(cpu, cxcz, multiply(cpu, sxsz, sy));
    store_single(cpu, output + 20, yy);

    if (info) {
        for (unsigned row = 0; row < 3; ++row)
            store_bits(cpu, output + 12 + 16 * row, read_be32(transform + 20 + 4 * row));
        cpu->fpr[0] = cpu->ps1[0] = f64_value(convert_to_double(read_be32(transform + 28)));
        set_single(cpu, 1, sxsz);
        set_single(cpu, 2, cxcz);
        set_single(cpu, 3, sy);
        set_single(cpu, 4, cxsz);
        set_single(cpu, 5, sz);
        set_single(cpu, 6, cz);
        set_single(cpu, 7, sxcz);
        cpu->gpr[5] = shift;
        cpu->gpr[6] = sin_table;
        cpu->gpr[7] = cos_table;
    } else {
        /* stfs rounds the incoming double translation arguments. Preserve
         * their registers, including non-single values and NaN payloads. */
        for (unsigned row = 0; row < 3; ++row)
            store_bits(cpu, output + 12 + 16 * row, convert_to_single(f64_bits(cpu->fpr[1 + row])));
        set_single(cpu, 0, yy);
        set_single(cpu, 4, sxsz);
        set_single(cpu, 5, cxcz);
        set_single(cpu, 6, sy);
        set_single(cpu, 7, cxsz);
        set_single(cpu, 8, sz);
        set_single(cpu, 9, cz);
        set_single(cpu, 10, sxcz);
        cpu->gpr[3] = shift;
        cpu->gpr[7] = sin_table;
        cpu->gpr[8] = cos_table;
    }
    cpu->gpr[0] = z;
    cpu->xer &= ~0x20000000u; /* the zero-extended sraw operands never carry */
    cpu->fpscr &= ~(BW_FP_FPSCR_FI | BW_FP_FPSCR_FR);
    bw_fp_fprf(cpu, bw_fp_class32(yy));
    cpu->downcount -= cycles;
    cpu->cycle_observation_suffix = 1;
    cpu->pc = cpu->lr & ~3u;
    ++runs[which];
    return 1;

decline:
    ++declined[which];
    return 0;
}
