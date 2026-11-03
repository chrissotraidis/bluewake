#ifndef BLUEWAKE_FPS_WATCH_H
#define BLUEWAKE_FPS_WATCH_H

#include "core/cpu.h"

// A [fps-dip] line when presentation falls below 95% of the selected 30/60/120
// FPS mode: the frames shown, the game's speed, how many game frames
// got an in-between frame, the draws rejected or unmatched, the stage, room and
// Link's position, and why (the game below full speed, frames not
// interpolated, or presents late). Smooth Motion counts as a dip whenever fewer than
// 90% of game frames were interpolated, even if repeated frames keep the shown count
// at the target. BLUEWAKE_FPS_WATCH=0 turns it off.

void bluewake_fps_watch_attach(CPUState* cpu);
// Once per retrace.
void bluewake_fps_watch_retrace(void);

// Pure classification shared with the synthetic regression. NULL means no dip.
const char* bluewake_fps_watch_reason(double shown, double speed, bool smooth,
                                    int steps, unsigned long long frames,
                                    unsigned long long interpolated);

// Which part held a second below target, from the game's speed (1.0 = full),
// that second's CPU use (percent of one core) and the game thread's waits
// (milliseconds): "shader-compile" (it waited on the GX worker while pipelines
// were made), "gx-worker", "gpu-present", "render-worker", "interp-helper",
// "game-thread" (the game ran slow without waiting on the others: its own work,
// or a hitch on its thread) or "unclear". Pure, shared with the regression.
const char* bluewake_fps_watch_cause(double speed, double game_busy, double gx_worker, double interp_helper,
                                    double render_worker, double gx_wait_ms, double present_ms,
                                    unsigned pipelines);

// Cumulative worker counters may reset when a worker exits or is replaced.
double bluewake_fps_watch_cpu_percent(unsigned long long current,
                                     unsigned long long previous,
                                     unsigned long long wall_us);

#endif
