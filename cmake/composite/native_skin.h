#ifndef BLUEWAKE_NATIVE_SKIN_H
#define BLUEWAKE_NATIVE_SKIN_H

/* J3DModel::calcWeightEnvelopeMtx natively (native_skin.c). */

#include "core/cpu.h"

#define BLUEWAKE_NATIVE_SKIN_ENTRY 0x802EE67Cu

/* The native's exact single rounding leans on fma(). Where the module is built
 * without a hardware fused multiply-add (x86 below x86-64-v3), fma() is a slow
 * library call and the native measured no faster than the translation, so it
 * stays off there unless BLUEWAKE_NATIVE_SKIN=1 asks for it. */
#if defined(__FMA__) || defined(__aarch64__) || defined(__ARM_FEATURE_FMA)
#define BLUEWAKE_NATIVE_SKIN_HARDWARE_FMA 1
#else
#define BLUEWAKE_NATIVE_SKIN_HARDWARE_FMA 0
#endif

typedef bool (*BluewakeNativeSkinReady)(void*, const CPUState*, u32);
int bluewake_composite_native_skin_v1(bool enabled, BluewakeNativeSkinReady ready, void* user);
/* 1: every envelope ran (go on at 0x802EE834); 2: the first ones (go on at 0x802EE6F0); 0: declined. */
int bluewake_native_skin_try(CPUState* cpu);

/* The function from its entry, with the return address in LR, through its
 * envelope loop: one with every register, flag, cycle and byte of memory as
 * the translation leaves them at the loop's test 0x802EE830 when every
 * envelope is done, the caller then taking the epilogue at 0x802EE834; two,
 * with BLUEWAKE_NATIVE_SKIN_PARTIAL=1, when only the first envelopes fit the
 * turn, the caller then taking the loop branch to 0x802EE6F0; zero, with
 * nothing changed, where that is not certain (the translation then runs).
 * No identifier here may be `ctx`. */
int bluewake_native_skin(CPUState* cpu);
void bluewake_native_skin_report(void);

#endif
