#ifndef BLUEWAKE_NATIVE_GROUND_WALK_H
#define BLUEWAKE_NATIVE_GROUND_WALK_H

/* cBgW::GroundCrossGrpRp (GZLE01 0x802491F4), the ground check's walk over a
 * collision object's groups, natively (native_ground_walk.c, #592). It plays
 * the same rather than reproducing every guest-visible effect: see the .c. */

#include "core/cpu.h"

#define BLUEWAKE_GROUND_WALK_ENTRY 0x802491F4u

/* At the function's entry label, LR the return address and r3-r6 its
 * arguments: nonzero when the whole call ran natively (r3 the result, r1 and
 * LR as the caller left them, the cycles charged); zero, with nothing changed,
 * where the native declines and the translation runs. Off unless
 * BLUEWAKE_NATIVE_GROUND_WALK=1. No identifier here may be `ctx`. */
int bluewake_native_ground_walk_try(CPUState* cpu);

#endif
