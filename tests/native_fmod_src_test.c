/* Finite __ieee754_fmod vectors as IEEE bits, and declines that leave fpr alone.
 * No disc. Compile the shim directly, not through the builder:
 *   cc -std=c11 -Wall -Wextra -Werror -O2 \
 *     -I cmake/composite -I "$GXRUNTIME/include" \
 *     -o native_fmod_src_test \
 *     tests/native_fmod_src_test.c \
 *     cmake/composite/native_fmod_src.c cmake/composite/e_fmod.c -lm
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_fmod_src.h"
#include "ieee754_be.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#define BW_HAVE_MXCSR 1
#endif

static uint64_t bits_of(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static double from_bits(uint64_t bits) {
    double value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static CPUState prepared(uint64_t xbits, uint64_t ybits) {
    CPUState cpu;
    memset(&cpu, 0x5a, sizeof cpu);
    cpu.fpr[1] = from_bits(xbits);
    cpu.fpr[2] = from_bits(ybits);
    cpu.lr = 0x8000ABC3u;
    cpu.pc = 0x80100000u;
    return cpu;
}

static void expect_finite(uint64_t xbits, uint64_t ybits, uint64_t want) {
    CPUState cpu = prepared(xbits, ybits);
    CPUState before = cpu;
    assert(bluewake_native_fmod_src(&cpu) == 1);
    assert(bits_of(cpu.fpr[1]) == want);
    assert(cpu.pc == (before.lr & ~3u));
    cpu.fpr[1] = before.fpr[1];
    cpu.pc = before.pc;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
}

static void expect_decline(uint64_t xbits, uint64_t ybits) {
    CPUState cpu = prepared(xbits, ybits);
    CPUState before = cpu;
    assert(bluewake_native_fmod_src(&cpu) == 0);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
}

static void test_words(void) {
    double value = from_bits(0x0123456789abcdefULL);
    assert((uint32_t)__HI(value) == 0x01234567u);
    assert((uint32_t)__LO(value) == 0x89abcdefu);
    bw_f64_set_hi(&value, (int)0x40000000);
    assert(bits_of(value) == 0x4000000089abcdefULL);
    bw_f64_set_lo(&value, 0u);
    assert(bits_of(value) == 0x4000000000000000ULL);
    value = from_bits(0xc010000000000000ULL);
    assert((uint32_t)__HI(value) == 0xc0100000u);
    assert((uint32_t)__LO(value) == 0u);
}

#ifdef BW_HAVE_MXCSR
static void expect_finite_under_ftz(uint64_t xbits, uint64_t ybits, uint64_t want) {
    unsigned saved = _mm_getcsr();
    unsigned flushed = saved | 0x8040u;
    _mm_setcsr(flushed);
    expect_finite(xbits, ybits, want);
    assert(_mm_getcsr() == flushed);
    _mm_setcsr(saved);
}

static void expect_decline_under_ftz(uint64_t xbits, uint64_t ybits) {
    unsigned saved = _mm_getcsr();
    unsigned flushed = saved | 0x8040u;
    _mm_setcsr(flushed);
    expect_decline(xbits, ybits);
    assert(_mm_getcsr() == flushed);
    _mm_setcsr(saved);
}
#endif

int main(void) {
    static const uint64_t finite[][3] = {
        {0x4014000000000000ULL, 0x4000000000000000ULL, 0x3ff0000000000000ULL}, /* 5, 2 -> 1 */
        {0x4014000000000000ULL, 0x4008000000000000ULL, 0x4000000000000000ULL}, /* 5, 3 -> 2 */
        {0xc016000000000000ULL, 0x4000000000000000ULL, 0xbff8000000000000ULL}, /* -5.5, 2 -> -1.5 */
        {0x4016000000000000ULL, 0xc000000000000000ULL, 0x3ff8000000000000ULL}, /* 5.5, -2 -> 1.5 */
        {0x3ff8000000000000ULL, 0x4010000000000000ULL, 0x3ff8000000000000ULL}, /* 1.5, 4 -> 1.5 */
        {0x4010000000000000ULL, 0x4010000000000000ULL, 0x0000000000000000ULL}, /* 4, 4 -> +0 */
        {0xc010000000000000ULL, 0x4010000000000000ULL, 0x8000000000000000ULL}, /* -4, 4 -> -0 */
        {0x4010000000000000ULL, 0xc010000000000000ULL, 0x0000000000000000ULL}, /* 4, -4 -> +0 */
        {0x0000000000000000ULL, 0x3ff0000000000000ULL, 0x0000000000000000ULL}, /* +0, 1 -> +0 */
        {0x8000000000000000ULL, 0x3ff0000000000000ULL, 0x8000000000000000ULL}, /* -0, 1 -> -0 */
        {0x4024000000000000ULL, 0x4008000000000000ULL, 0x3ff0000000000000ULL}, /* 10, 3 -> 1 */
        {0x4022000000000000ULL, 0x4004000000000000ULL, 0x3ff8000000000000ULL}, /* 9, 2.5 -> 1.5 */
        {0x4340000000000000ULL, 0x4008000000000000ULL, 0x4000000000000000ULL}, /* 2^53, 3 -> 2 */
    };
    static const uint64_t decline[][2] = {
        {0x3ff0000000000000ULL, 0x0000000000000000ULL}, /* y = +0 */
        {0x3ff0000000000000ULL, 0x8000000000000000ULL}, /* y = -0 */
        {0x7ff8000000000001ULL, 0x3ff0000000000000ULL}, /* x = NaN */
        {0x3ff0000000000000ULL, 0x7ff8000000000001ULL}, /* y = NaN */
        {0x7ff0000000000001ULL, 0x4000000000000000ULL}, /* x = signaling NaN */
        {0x4000000000000000ULL, 0x7ff0000000000001ULL}, /* y = signaling NaN */
        {0x7ff0000000000000ULL, 0x3ff0000000000000ULL}, /* x = +inf */
        {0xfff0000000000000ULL, 0x3ff0000000000000ULL}, /* x = -inf */
        {0x3ff0000000000000ULL, 0x7ff0000000000000ULL}, /* y = +inf */
        {0x3ff0000000000000ULL, 0xfff0000000000000ULL}, /* y = -inf */
        {0x7ff0000000000000ULL, 0x7ff0000000000000ULL}, /* inf, inf */
        {0x7ff8000000000001ULL, 0x0000000000000000ULL}, /* NaN, 0 */
        {0x0000000000000001ULL, 0x3ff0000000000000ULL}, /* x = smallest subnormal */
        {0x3ff0000000000000ULL, 0x000fffffffffffffULL}, /* y = largest subnormal */
        {0x8000000000000001ULL, 0x4000000000000000ULL}, /* x = -subnormal */
        {0x0010000000000001ULL, 0x0010000000000000ULL}, /* normal inputs, subnormal result */
    };
    unsigned i;

    test_words();
    assert(bluewake_native_fmod_src(NULL) == 0);
    for (i = 0; i < sizeof finite / sizeof finite[0]; ++i)
        expect_finite(finite[i][0], finite[i][1], finite[i][2]);
    for (i = 0; i < sizeof decline / sizeof decline[0]; ++i)
        expect_decline(decline[i][0], decline[i][1]);
#ifdef BW_HAVE_MXCSR
    expect_finite_under_ftz(0x4014000000000000ULL, 0x4000000000000000ULL, 0x3ff0000000000000ULL);
    expect_decline_under_ftz(0x0010000000000001ULL, 0x0010000000000000ULL);
    expect_decline_under_ftz(0x0000000000000001ULL, 0x3ff0000000000000ULL);
#endif
    printf("native_fmod_src_test: ok (%u finite, %u decline)\n",
           (unsigned)(sizeof finite / sizeof finite[0]),
           (unsigned)(sizeof decline / sizeof decline[0]));
    return 0;
}
