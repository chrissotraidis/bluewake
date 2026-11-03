#ifndef BLUEWAKE_NATIVE_GROUND_WALK_H
#define BLUEWAKE_NATIVE_GROUND_WALK_H

/* cBgW::GroundCrossGrpRp (GZLE01 0x802491F4-0x80249368) natively, opt-in
 * (#542: BLUEWAKE_NATIVE_GROUND_WALK=1). See native_ground_walk.c. */

#include "core/cpu.h"

#define BLUEWAKE_GROUND_WALK_ENTRY 0x802491F4u

typedef bool (*BluewakeGroundWalkReady)(void*, const CPUState*, u32);
int bluewake_composite_ground_walk_v1(bool enabled, BluewakeGroundWalkReady ready, void* user);

/* The chunk's hook at the function's entry label:
 *   0  declined, nothing changed: the translation runs from the entry;
 *   1  the call returned: pc is the return address (the hook takes the
 *      translation's return dispatch);
 *   2  the native stopped where the translation could return to the
 *      dispatcher (pc set there, every effect before it made): the hook
 *      returns to the dispatcher, which continues the translation.
 * No identifier here may be `ctx`. */
int bluewake_ground_walk_try(CPUState* cpu);
int bluewake_ground_walk(CPUState* cpu);
void bluewake_ground_walk_report(void);

#endif
