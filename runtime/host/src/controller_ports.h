// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Player 1 for a connected controller while no controller is player 1 (#61).
//
// BlueWake reads player 1 only, and Aurora reads a port from SDL's player
// slot. SDL gives a controller a slot when it detects it, and only if it
// recognizes it then. One it recognizes later, through gamecontrollerdb.txt
// (loaded once SDL is already running), is opened but never placed, so it did
// nothing; so did a sole controller left in slot 2 after the first one was
// unplugged. Call this whenever a controller is added, removed or remapped:
// with player 1 free, the first connected controller takes it.
//
// The choice is not saved. PADSetPortForIndex would save it, and a saved port
// then keeps every other controller off player 1 until that one returns.
#include <stdbool.h>
#include <stdio.h>
#include <dolphin/pad.h>
#include <SDL3/SDL_gamepad.h>

// The mapping SDL chose for a controller, in the session log: with a mapping
// from gamecontrollerdb.txt, SDL uses a line with a crc: field only when the
// controller's CRC matches, and otherwise one without, so the line a player
// added may not be the one in use (#61).
static inline void bw_log_gamepad_mapping(SDL_JoystickID id) {
    char* mapping = SDL_GetGamepadMappingForID(id);
    if (mapping != NULL) {
        fprintf(stderr, "[pad] mapping in use: %s\n", mapping);
        SDL_free(mapping);
    }
}

static inline bool bw_claim_player_one(void) {
    if (PADGetIndexForPort(0) >= 0)
        return false;
    const u32 count = PADCount();
    for (u32 i = 0; i < count; i++) {
        SDL_Gamepad* pad = PADGetSDLGamepadForIndex(i);
        if (pad == NULL)
            continue;
        const int previous = SDL_GetGamepadPlayerIndex(pad);
        if (!SDL_SetGamepadPlayerIndex(pad, 0))
            continue;
        const char* name = SDL_GetGamepadName(pad);
        if (previous < 0)
            fprintf(stderr, "[pad] '%s' is player 1 (it had no player slot)\n", name ? name : "controller");
        else
            fprintf(stderr, "[pad] '%s' is player 1 (it was player %d)\n", name ? name : "controller",
                    previous + 1);
        return true;
    }
    return false;
}

// The game's own stick clamp as the only dead zone (#138). Aurora cut each axis
// below 8000 of 32767 (24%) and did not rescale what was left, and the game's
// GameCube clamp subtracted its dead zone after that: the first value the game
// saw was 22% of its range, and it reached full tilt by two thirds of the
// stick's travel. With the cutoff off, the runtime scales the stick's full
// travel to a GameCube stick's (RecompCore pad.cpp, gamecube_axis), so the
// game's clamp works as on the console. Loads the controller's saved mapping
// first: loading it later would put the cutoff back.
static inline void bw_game_dead_zone(unsigned port) {
    u32 count = 0;
    (void)PADGetButtonMappings(port, &count);
    PADDeadZones* zones = PADGetDeadZones(port);
    if (zones != NULL && zones->useDeadzones) {
        zones->useDeadzones = false;
        fprintf(stderr, "[pad] player %u: the game's own stick dead zone\n", port + 1u);
    }
}
