#ifndef BLUEWAKE_QUICK_DOORS_H
#define BLUEWAKE_QUICK_DOORS_H

#include "core/cpu.h"

#include <stdbool.h>

// Quick doors: going through a door with a knob (houses, shops and the other
// interiors), Link opens it as the game has him do, and once the door's own
// fade covers the screen the rest is cut: his walk to the exit behind it, the
// wait for the fade, and in the next room the door opening again and closing
// behind him. He stands just inside the door, closed behind him, and can move
// as the picture comes back. See quick_doors.c.
//
//   BLUEWAKE_QUICK_DOORS=0              the game's doors
//   BLUEWAKE_DOOR_TRACE=1               log what is cut, and each door's timing
//   BLUEWAKE_DOOR_TRACE=2               also log state every retrace
//   BLUEWAKE_DOOR_TEST_FACE=r:x:z,...   testing: turn Link to face (x, z) at retrace r

// Once the guest is running.
void bluewake_quick_doors_attach(CPUState* cpu);
// Once per retrace.
void bluewake_quick_doors_retrace(void);
// Reads BLUEWAKE_QUICK_DOORS again (the options menu).
void bluewake_quick_doors_reload(void);
// A door's own fade covers the screen and the scene change it leads to is
// under way: fast_load.c's fast-forward may run (as in a scene change's black).
bool bluewake_quick_doors_covered(void);
bool bluewake_quick_doors_busy(void);

// At every dispatch boundary (the chassis edge service), while a scene change
// runs (armed): at the new stage's player creation, a knob door's start point
// is made an ordinary one. Only memory is changed (never the pc), and only a
// flag is read while nothing is armed.
#define BLUEWAKE_QUICK_DOORS_ACTOR_CREATE 0x80041628u // dStage_actorCreate
extern bool bluewake_quick_doors_armed;
void bluewake_quick_doors_enter(CPUState* cpu);
static inline bool bluewake_quick_doors_observes(u32 address) {
    return bluewake_quick_doors_armed && address == BLUEWAKE_QUICK_DOORS_ACTOR_CREATE;
}
static inline void bluewake_quick_doors_dispatch(CPUState* cpu, u32 address) {
    if (__builtin_expect(bluewake_quick_doors_observes(address), 0))
        bluewake_quick_doors_enter(cpu);
}

#endif
