#ifndef BLUEWAKE_SPRINT_H
#define BLUEWAKE_SPRINT_H

#include "core/cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

// Holding Shift makes Link run faster than his normal top speed, his run
// animation sped up to match. On a controller, clicking the left stick starts
// the sprint; it lasts until Link stops (the stick back in the middle) or the
// next click.
//
//   BLUEWAKE_SPRINT_SPEED=1.5          how much faster (1: off)
//   BLUEWAKE_SPRINT_TRACE=1            log it and Link's speed
//   BLUEWAKE_SPRINT_TEST=retrace:n     testing: Shift held for n retraces

void bluewake_sprint_attach(CPUState* cpu);
// Once per retrace, on the thread that pumps SDL's events.
void bluewake_sprint_retrace(void);
void bluewake_sprint_touch(bool down);
// Reads BLUEWAKE_SPRINT_SPEED again (the options menu).
void bluewake_sprint_reload(void);

#ifdef __cplusplus
}
#endif

#endif
