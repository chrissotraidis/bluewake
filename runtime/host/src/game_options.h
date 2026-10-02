#ifndef BLUEWAKE_GAME_OPTIONS_H
#define BLUEWAKE_GAME_OPTIONS_H

#include "core/cpu.h"

#include <stdbool.h>

// Game options (mods/betterww/options.txt, docs/MODS.md): Better Wind Waker's
// settings as switches. The option sites in the composite read the switches;
// this sets them, runs the native code at the hook sites, writes the options'
// values and patches the message data for instant text.
//
// BLUEWAKE_OPTIONS adjusts the defaults: a comma-separated list of option
// names, each turned on, or off when it starts with '-'; "none" first turns
// every option off. The options take effect only while the mod that carries
// their sites ("betterww") is enabled.

// At boot, after the mods are enabled and before the first dispatch.
void bluewake_game_options_enable(void* lib, CPUState* cpu, bool mod_enabled);
// Once per retrace.
void bluewake_game_options_retrace(CPUState* cpu);
// The options the game module offers, for a settings menu: the name at a
// position (the setting's key), its title, whether it is on by default and
// whether it is on now; NULL past the last, or before the game has started.
const char* bluewake_game_options_describe(u32 position, const char** title, bool* default_on, bool* on);

#endif
