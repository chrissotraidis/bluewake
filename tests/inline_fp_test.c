/* cmake/composite/inline_fp.h against GXRuntime's interpreter: every inline
 * operation must leave exactly the state the interpreter function leaves -
 * the destination's two halves, every other register, the FPSCR and the CR -
 * for ordinary operands and for the special ones that take the fallback. */
#include "core/cpu.h"
#include "inline_fp.h"

#undef ppc_fadds
#undef ppc_fsubs
#undef ppc_fadd
#undef ppc_fsub
#undef ppc_fmuls
#undef ppc_fmul
#undef ppc_fdivs
#undef ppc_fcmp
#undef ppc_fma
#undef ppc_ps_madds0
#undef ppc_ps_madds1
#undef ppc_ps_madd_op
#undef ppc_ps_mul_op
#undef ppc_ps_muls0
#undef ppc_ps_muls1
#undef ppc_ps_add_op
#undef ppc_ps_sub_op
#undef ppc_ps_sum0
#undef ppc_ps_sum1

#include <math.h>
#include <stdio.h>
#include <string.h>

static u64 rng = 0x9E3779B97F4A7C15ull;
static u64 next(void) {
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

static f64 from_bits(u64 bits) {
    f64 value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

/* Operands the translated code sees: singles held as doubles, doubles,
 * denormals of both widths, zeros, huge values, infinities and NaNs. */
static f64 operand(void) {
    static const u64 special[] = {
        0x0000000000000000ull, 0x8000000000000000ull, 0x7FF0000000000000ull,
        0xFFF0000000000000ull, 0x7FF8000000000000ull, 0x7FF4000000000000ull,
        0x0000000000000001ull, 0x800FFFFFFFFFFFFFull, 0x7FEFFFFFFFFFFFFFull,
        0x3810000000000000ull, 0x380FFFFFFFFFFFFFull, 0x47EFFFFFE0000000ull,
        0x3FF0000000000000ull, 0xBFF0000000000000ull, 0x36A0000000000000ull,
    };
    switch (next() % 8u) {
    case 0:
        return from_bits(special[next() % (sizeof special / sizeof special[0])]);
    case 1:
        return from_bits(next());  /* any double */
    case 2: {
        u32 bits = (u32)next();
        f32 single;
        memcpy(&single, &bits, sizeof single);
        return (f64)single;  /* any single, NaNs and denormals included */
    }
    default:
        return (f64)(f32)((f64)(s32)next() / (f64)(1u + (next() % 100000u)));
    }
}

static void randomize(CPUState* cpu) {
    for (unsigned i = 0; i < 32; ++i) {
        cpu->fpr[i] = operand();
        cpu->ps1[i] = operand();
    }
    cpu->fpscr = (u32)next() & ~0x000000F8u;  /* exceptions never enabled... */
    if (next() % 4u == 0u)
        cpu->fpscr |= (u32)next() & 0x000000F8u;  /* ...except sometimes */
    cpu->cr = (u32)next();
    /* Exercise the guest-selected rounding/flush mode, not just FPSCR bits. */
    ppc_fpscr_control_updated(cpu);
}

static bool same(const CPUState* a, const CPUState* b) {
    return memcmp(a, b, sizeof *a) == 0;
}

static CPUState reference, candidate;
static unsigned failures;

static void report(const char* name, unsigned iteration) {
    if (failures++ < 10)
        fprintf(stderr, "%s differs at iteration %u: fpscr %08X/%08X cr %08X/%08X\n", name,
                iteration, reference.fpscr, candidate.fpscr, reference.cr, candidate.cr);
}

#define CHECK3(name, inline_fn, interp_fn)                                   \
    do {                                                                    \
        const u8 d = (u8)(next() % 32u), x = (u8)(next() % 32u), y = (u8)(next() % 32u); \
        randomize(&reference);                                              \
        candidate = reference;                                              \
        interp_fn(&reference, d, x, y);                                     \
        inline_fn(&candidate, d, x, y);                                     \
        if (!same(&reference, &candidate))                                  \
            report(name, i);                                                \
    } while (0)

#define CHECK4(name, inline_fn, interp_fn)                                   \
    do {                                                                    \
        const u8 d = (u8)(next() % 32u), x = (u8)(next() % 32u), y = (u8)(next() % 32u), \
                 z = (u8)(next() % 32u);                                    \
        randomize(&reference);                                              \
        candidate = reference;                                              \
        interp_fn(&reference, d, x, y, z);                                  \
        inline_fn(&candidate, d, x, y, z);                                  \
        if (!same(&reference, &candidate))                                  \
            report(name, i);                                                \
    } while (0)

/* Explicit zero/subnormal divisors under every guest rounding, NI and
 * divide/invalid exception-enable combination. Random FPSCR bits alone did
 * not exercise DAZ before the fixture synchronized the host mode. */
static bool division_edges(void) {
    static const u64 divisors[] = {
        0, 0x8000000000000000ull, 1, 0x8000000000000001ull,
        0x000FFFFFFFFFFFFFull, 0x800FFFFFFFFFFFFFull,
        0x0010000000000000ull, 0x8010000000000000ull,
    };
    static const u64 numerators[] = {
        0, 0x8000000000000000ull, 0x3FF0000000000000ull,
        0xBFF0000000000000ull, 1, 0x7FEFFFFFFFFFFFFFull,
    };
    for (u32 mode = 0; mode < 32; ++mode) {
        for (unsigned x = 0; x < sizeof numerators / sizeof numerators[0]; ++x) {
            for (unsigned y = 0; y < sizeof divisors / sizeof divisors[0]; ++y) {
                CPUState a = {0}, b;
                a.fpscr = (mode & 7u) | ((mode & 8u) ? 0x10u : 0u) |
                          ((mode & 16u) ? 0x80u : 0u);
                a.fpr[1] = from_bits(numerators[x]);
                a.fpr[2] = from_bits(divisors[y]);
                a.fpr[3] = 123.0;
                a.ps1[3] = -123.0;
                ppc_fpscr_control_updated(&a);
                b = a;
                ppc_fdivs(&a, 3, 1, 2);
                bw_fp_fdivs(&b, 3, 1, 2);
                if (!same(&a, &b)) {
                    fprintf(stderr, "fdivs edge differs: mode=%u numerator=%016llX divisor=%016llX\n",
                            mode, (unsigned long long)numerators[x], (unsigned long long)divisors[y]);
                    return false;
                }
            }
        }
    }
    CPUState reset = {0};
    ppc_fpscr_control_updated(&reset);
    return true;
}

int main(void) {
    if (!division_edges())
        return 1;
    if (!cpu_init(&reference))
        return 1;
    const unsigned iterations = 2000000u;
    for (unsigned i = 0; i < iterations; ++i) {
        CHECK4("ps_madds0", bw_fp_ps_madds0, ppc_ps_madds0);
        CHECK4("ps_madds1", bw_fp_ps_madds1, ppc_ps_madds1);
        CHECK4("ps_sum0", bw_fp_ps_sum0, ppc_ps_sum0);
        CHECK4("ps_sum1", bw_fp_ps_sum1, ppc_ps_sum1);
        CHECK3("ps_mul", bw_fp_ps_mul_op, ppc_ps_mul_op);
        CHECK3("ps_muls0", bw_fp_ps_muls0, ppc_ps_muls0);
        CHECK3("ps_muls1", bw_fp_ps_muls1, ppc_ps_muls1);
        CHECK3("ps_add", bw_fp_ps_add_op, ppc_ps_add_op);
        CHECK3("ps_sub", bw_fp_ps_sub_op, ppc_ps_sub_op);
        {
            const u8 d = (u8)(next() % 32u), x = (u8)(next() % 32u), y = (u8)(next() % 32u),
                     z = (u8)(next() % 32u);
            const bool subtract = (next() & 1u) != 0u, negative = (next() & 1u) != 0u;
            randomize(&reference);
            candidate = reference;
            ppc_ps_madd_op(&reference, d, x, y, z, subtract, negative);
            bw_fp_ps_madd_op(&candidate, d, x, y, z, subtract, negative);
            if (!same(&reference, &candidate))
                report("ps_madd", i);
        }
        {
            const bool single = (next() & 1u) != 0u, subtract = (next() & 1u) != 0u,
                       negative = (next() & 1u) != 0u;
            randomize(&reference);
            candidate = reference;
            const f64 a = operand(), c = operand(), b = operand();
            f64 out_reference = 0.0, out_candidate = 0.0;
            const bool ok_reference = ppc_fma(&reference, a, c, b, single, subtract, negative, &out_reference);
            const bool ok_candidate = bw_fp_fma(&candidate, a, c, b, single, subtract, negative, &out_candidate);
            if (!same(&reference, &candidate) || ok_reference != ok_candidate ||
                memcmp(&out_reference, &out_candidate, sizeof out_reference) != 0)
                report("fma", i);
        }
        CHECK3("fadds", bw_fp_fadds, ppc_fadds);
        CHECK3("fsubs", bw_fp_fsubs, ppc_fsubs);
        CHECK3("fadd", bw_fp_fadd, ppc_fadd);
        CHECK3("fsub", bw_fp_fsub, ppc_fsub);
        CHECK3("fmuls", bw_fp_fmuls, ppc_fmuls);
        CHECK3("fmul", bw_fp_fmul, ppc_fmul);
        CHECK3("fdivs", bw_fp_fdivs, ppc_fdivs);
        {
            const u8 crfd = (u8)(next() % 8u);
            const bool ordered = (next() & 1u) != 0u;
            randomize(&reference);
            candidate = reference;
            const f64 a = operand(), b = operand();
            ppc_fcmp(&reference, crfd, a, b, ordered);
            bw_fp_fcmp(&candidate, crfd, a, b, ordered);
            if (!same(&reference, &candidate))
                report("fcmp", i);
        }
    }
    reference.fpscr = 0;
    ppc_fpscr_control_updated(&reference);
    cpu_free(&reference);
    if (failures != 0u) {
        fprintf(stderr, "inline_fp: %u differences\n", failures);
        return 1;
    }
    printf("inline_fp: %u iterations of 19 operations match the interpreter\n", iterations);
    return 0;
}
