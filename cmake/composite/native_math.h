#ifndef BLUEWAKE_NATIVE_MATH_H
#define BLUEWAKE_NATIVE_MATH_H
#include "core/cpu.h"
typedef bool (*BluewakeNativeMathReady)(void*, const CPUState*, u32);
int bluewake_composite_native_math_v1(bool enabled, BluewakeNativeMathReady ready, void* user);
int bluewake_native_math_try(CPUState* cpu, u32 address);
// Returns zero without changing state when exact replacement is unavailable.
int bluewake_native_math(CPUState* cpu, u32 address);
int bluewake_native_gpr(CPUState* cpu, u32 address);
void bluewake_native_math_report(void);
#endif
