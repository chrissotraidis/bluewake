#ifndef BLUEWAKE_FPS_WATCH_H
#define BLUEWAKE_FPS_WATCH_H

#include "core/cpu.h"

// A [fps-dip] line when presentation falls below 95% of the selected 30/60/120
// FPS mode: the frames shown, the game's speed, how many game frames
// got an in-between frame, the draws rejected or unmatched, the stage, room and
// Link's position, and why (the game below full speed, frames not
// interpolated, or presents late). BLUEWAKE_FPS_WATCH=0 turns it off.

void bluewake_fps_watch_attach(CPUState* cpu);
// Once per retrace.
void bluewake_fps_watch_retrace(void);

// Pure classification shared with the synthetic regression. NULL means no dip.
const char* bluewake_fps_watch_reason(double shown, double speed, bool smooth,
                                    int steps, unsigned long long frames,
                                    unsigned long long interpolated);

// Cumulative worker counters may reset when a worker exits or is replaced.
double bluewake_fps_watch_cpu_percent(unsigned long long current,
                                     unsigned long long previous,
                                     unsigned long long wall_us);

#endif
