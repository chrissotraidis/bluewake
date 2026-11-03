// SPDX-License-Identifier: GPL-3.0-or-later
// The desktop menu's settings core (desktop_settings.cpp), as the menu's own
// drawing (desktop_menu.cpp) sees it: the next-launch choices it keeps a copy
// of, and what a restart would change. The rest is desktop_menu.h's.
//
// No ImGui, SDL or Aurora here or in desktop_settings.cpp, so the host tests
// compile the real core (tests/test_safe_mode_hd.py).
#pragma once
#include "desktop_menu.h"
#include <string>
#include <utility>
#include <vector>

namespace bw_desktop {

// The next-launch choices the session has already read (the picture's shape,
// the mods and their options, the texture pack): changed in the menu, written
// to the file, never to this session's variables, which the game, the mods and
// Aurora have already read.
struct LaunchChoices {
    std::string aspect = "4:3";
    bool betterww = false;
    std::vector<std::pair<std::string, bool>> options;  // Better Wind Waker's: name, on
    std::vector<std::string> option_titles;
    std::string texture_pack;  // the HD pack's folder, "" when HD textures are off
    std::string last_pack;     // the folder to come back to when they go on again
};
LaunchChoices& launch_choices();
// Reads them, the first time the menu opens or the file is saved: later
// openings keep what the player chose since.
void capture_launch_choices();
// One of them changed in the menu: saved soon.
void choice_changed(const char* key);
// HD textures on with `folder`, or off with "" (the folder is kept to come back to).
void set_texture_pack(const std::string& folder);

// Whether this session takes a setting at the next launch, on this platform
// (settings_table.h's next_launch).
bool next_launch(const bw_settings::Setting& s);
// The launch is complete: what a restart would change is measured from here.
void note_launch();
// Whether restarting would change anything.
bool needs_restart();

bool safe_backed_up();  // safe mode copied the settings file aside first
// A change is a second old and not written yet (hotkeys and the window: they
// are saved a second after the last one).
bool save_due();

}  // namespace bw_desktop
