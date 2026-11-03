#ifndef BLUEWAKE_NATIVE_FMOD_SRC_H
#define BLUEWAKE_NATIVE_FMOD_SRC_H

/* __ieee754_fmod compiled from e_fmod.c. One function.
 * x is fpr[1], y is fpr[2], the remainder is written to fpr[1].
 * Returns 1 only after pc is set from lr the way native_math.c does
 * (pc = lr with the low two bits cleared).
 * Returns 0 with every register left alone when y is 0, or when an input
 * or the result is NaN, infinite, or subnormal.
 * This file does not hook label 0x8032EC1C. */

#include "core/cpu.h"

int bluewake_native_fmod_src(CPUState* cpu);

#endif
