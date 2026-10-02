#ifndef BLUEWAKE_NATIVE_SKIN_H
#define BLUEWAKE_NATIVE_SKIN_H

/* J3DModel::calcWeightEnvelopeMtx natively (native_skin.c). */

#include "core/cpu.h"

#define BLUEWAKE_NATIVE_SKIN_ENTRY 0x802EE67Cu

extern int bluewake_native_skin_enabled;

/* The whole function, from its entry with the return address in LR, to the
 * return: nonzero with every register, flag, cycle and byte of memory as the
 * translated function leaves them; zero, with nothing changed, where that is
 * not certain (the translation then runs). No identifier here may be `ctx`. */
int bluewake_native_skin(CPUState* cpu);
void bluewake_native_skin_report(void);

#endif
