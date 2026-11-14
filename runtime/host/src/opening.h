#ifndef BLUEWAKE_OPENING_H
#define BLUEWAKE_OPENING_H

#include "core/cpu.h"

#include <stdbool.h>

// The opening legend in widescreen, with Native cutscene framing (#597). The
// widescreen codes widen the game's 2D port, so 4:3 screens keep their shape
// in the middle; but the 16:9 code (Dolphin's) also has the legend's screen
// (Opening.blo: the story panels and their subtitles) draw with no port (li
// r4,0 at 0x80234528, before dScnOpen_proc_c::proc_draw's J2DScreen::draw), so
// it makes its own 640x480 one over the whole window and the legend fills the
// screen stretched, text and all. With Native cutscene framing on, that draw is
// handed the game's widened port back, so the legend keeps its shape, and the
// screen clips to its own 640x480, so the sides stay black as at 4:3: the
// legend's layout is a scroll wider and taller than the screen, whose black
// bands are only the screen's width, and through the whole widened port its
// text columns showed past them. See opening.c.
//
//   BLUEWAKE_NATIVE_CUTSCENE_FRAMING=1       on (off by default; letterbox.h)
//   BLUEWAKE_LEGEND_TRACE=1       log the first draw it hands the port to

// Once the guest is running (the widescreen code is in by then).
void bluewake_opening_attach(CPUState* cpu);
// Reads BLUEWAKE_NATIVE_CUTSCENE_FRAMING again (the options menu).
void bluewake_opening_reload(void);

// At every dispatch boundary (the chassis edge service): J2DScreen::draw's
// entry, called from the legend (its return address) with no port in r4.
// Only a flag is read while it is off.
#define BLUEWAKE_OPENING_SCREEN_DRAW 0x802D0F2Cu  // draw__9J2DScreenFffPC14J2DGrafContext
#define BLUEWAKE_OPENING_DRAW_RETURN 0x80234530u  // proc_draw's call (0x8023452C) returns here
extern bool bluewake_opening_armed;
void bluewake_opening_hook(CPUState* cpu);
static inline bool bluewake_opening_observes(u32 address) {
    return bluewake_opening_armed && address == BLUEWAKE_OPENING_SCREEN_DRAW;
}
static inline void bluewake_opening_dispatch(CPUState* cpu, u32 address) {
    if (__builtin_expect(bluewake_opening_observes(address), 0))
        bluewake_opening_hook(cpu);
}

#endif
