#ifndef BLUEWAKE_TURBO_H
#define BLUEWAKE_TURBO_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Holding Tab — or a controller's right stick click, which the GameCube
// mapping leaves free — runs the game faster than realtime: each retrace is
// paced that much closer to the last, and the sound is skipped while it is
// held (see turbo.c for why that is the half that decides the speed).
//
//   BLUEWAKE_TURBO=1                   the switch (the menu's, settings.ini)
//   BLUEWAKE_TURBO_SPEED=3             how many times realtime (1: off)

void bluewake_turbo_attach(void);
// Once per retrace, on the thread that pumps SDL's events.
void bluewake_turbo_retrace(void);
// Reads BLUEWAKE_TURBO and BLUEWAKE_TURBO_SPEED again (the options menu).
void bluewake_turbo_reload(void);
// Whether the game is speeding up now: the pace is divided, the sound skipped.
bool bluewake_turbo_active(void);
// The factor as a percentage, 100 when not speeding up (the pace's arithmetic).
u32 bluewake_turbo_percent(void);

#ifdef __cplusplus
}
#endif

#endif
