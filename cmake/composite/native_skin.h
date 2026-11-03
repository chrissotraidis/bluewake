#ifndef BLUEWAKE_NATIVE_SKIN_H
#define BLUEWAKE_NATIVE_SKIN_H

/* J3DModel::calcWeightEnvelopeMtx natively (native_skin.c). */

#include "core/cpu.h"

#define BLUEWAKE_NATIVE_SKIN_ENTRY 0x802EE67Cu

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
