#ifndef BLUEWAKE_IEEE754_BE_H
#define BLUEWAKE_IEEE754_BE_H

/* Big-endian IEEE-754 word access for e_fmod.c.
 * High word is bits 63..32 of the value, low word is bits 31..0.
 * The words are shifts of a uint64_t copied with memcpy, so a little-endian
 * host does not treat the first addressed int as the high word.
 * Do not include fdlibm.h. Its pointer pun reads the low 32 bits on x86_64.
 * __HI and __LO are rvalues. Stores go through bw_f64_set_hi and bw_f64_set_lo. */

#include <assert.h>
#include <stdint.h>
#include <string.h>

static_assert(sizeof(int) == 4, "ieee754 words are 32-bit int");

static inline uint64_t bw_f64_bits(double x) {
    uint64_t bits;
    memcpy(&bits, &x, sizeof bits);
    return bits;
}

static inline int bw_f64_hi(double x) {
    return (int)(uint32_t)(bw_f64_bits(x) >> 32);
}

static inline int bw_f64_lo(double x) {
    return (int)(uint32_t)bw_f64_bits(x);
}

static inline void bw_f64_set_hi(double* x, int hi) {
    uint64_t bits = bw_f64_bits(*x);
    bits = (bits & 0x00000000ffffffffULL) | ((uint64_t)(uint32_t)hi << 32);
    memcpy(x, &bits, sizeof bits);
}

static inline void bw_f64_set_lo(double* x, unsigned lo) {
    uint64_t bits = bw_f64_bits(*x);
    bits = (bits & 0xffffffff00000000ULL) | (uint64_t)(uint32_t)lo;
    memcpy(x, &bits, sizeof bits);
}

#define __HI(x) bw_f64_hi(x)
#define __LO(x) bw_f64_lo(x)

#endif
