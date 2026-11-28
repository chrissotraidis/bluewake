// SPDX-License-Identifier: GPL-3.0-or-later
// Swift Sail's steadier boat (mods/betterww/options.txt, hook 7 in
// daShip_c::setYPos). The game lets King of Red Lions ride up to 40 units above
// the water and sink 60 below it while a spring pulls him toward it. At the
// game's top speed (55) that spring keeps up; at Swift Sail's (110) the swells
// pass twice as fast, it overshoots, and he sinks far enough for the sea to
// show through the deck. With Swift Sail on he sinks no more than
// SAIL_STEADY_DEPTH below the water, and a sink stopped there stops pulling
// down. Only his drawn height on the water changes: speed, steering, the wind
// and collisions are the game's.
#ifndef BLUEWAKE_SAIL_STEADY_H
#define BLUEWAKE_SAIL_STEADY_H

#define SAIL_STEADY_DEPTH 12.0f

// y: the boat's height (current.pos.y); water: the water's height under him
// (m03F4); sink: his height's own speed (m03D0, units a frame). Returns 1 when
// it held him.
static inline int sail_steady(float* y, float water, float* sink) {
    const float lowest = water - SAIL_STEADY_DEPTH;
    if (!(*y < lowest))
        return 0;
    *y = lowest;
    if (*sink < 0.0f)
        *sink = 0.0f;
    return 1;
}

#endif
