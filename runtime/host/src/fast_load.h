#ifndef BLUEWAKE_FAST_LOAD_H
#define BLUEWAKE_FAST_LOAD_H

#include "core/cpu.h"

// Scene changes (a door, an exit, loading a save): the fades take
// BLUEWAKE_FADE_FRAMES game frames each way (default 6, 0.2 s) instead of the
// game's 26, and the black between them runs as fast as the host can.
//
//   BLUEWAKE_FADE_FRAMES=6                              game frames a fade takes (0: the game's)
//   BLUEWAKE_FAST_FORWARD=0                             run the black at normal speed
//   BLUEWAKE_LOAD_TRACE=1                               log each retrace of a scene change
//   BLUEWAKE_TEST_WARP=retrace:stage:room:point[:layer] testing: ask for a scene change

void bluewake_fast_load_attach(CPUState* cpu);
// Once per retrace, with the emulation thread's CPU time in microseconds.
void bluewake_fast_load_retrace(unsigned long long cpu_us);
// While a scene change's black is fast-forwarded: no wall-clock pacing.
bool bluewake_fast_load_fast_forward(void);
// Reads BLUEWAKE_FADE_FRAMES and BLUEWAKE_FAST_FORWARD again (the options menu).
void bluewake_fast_load_reload(void);

#endif
