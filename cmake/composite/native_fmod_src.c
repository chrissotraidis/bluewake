#include "native_fmod_src.h"

#include <fenv.h>
#include <string.h>

double __ieee754_fmod(double x, double y);

static uint64_t magnitude(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits & 0x7fffffffffffffffULL;
}

static int biased_exponent(uint64_t mag) {
    return (int)((mag >> 52) & 0x7ffu);
}

/* Zero declines only for the divisor. A zero dividend is an ordinary result. */
static int decline_operand(double value, int decline_zero) {
    uint64_t mag = magnitude(value);
    int exp = biased_exponent(mag);
    if (mag == 0)
        return decline_zero;
    return exp == 0 || exp == 0x7ff;
}

/* A zero remainder is exact. Subnormal, infinite, and NaN results are not taken. */
static int decline_result(double value) {
    uint64_t mag = magnitude(value);
    int exp = biased_exponent(mag);
    if (mag == 0)
        return 0;
    return exp == 0 || exp == 0x7ff;
}

int bluewake_native_fmod_src(CPUState* cpu) {
    double x, y, result;
    fenv_t env;

    if (cpu == NULL)
        return 0;
    x = cpu->fpr[1];
    y = cpu->fpr[2];
    if (decline_operand(x, 0) || decline_operand(y, 1))
        return 0;

    /* The game thread runs flush-to-zero. Turn that off for the call so a
     * subnormal remainder is still visible, then decline it. Restoring the
     * environment also drops any host exception the C file raised. */
    if (fegetenv(&env) != 0)
        return 0;
    if (fesetenv(FE_DFL_ENV) != 0)
        return 0;
    result = __ieee754_fmod(x, y);
    fesetenv(&env);

    if (decline_result(result))
        return 0;

    cpu->fpr[1] = result;
    cpu->pc = cpu->lr & ~3u;
    return 1;
}
