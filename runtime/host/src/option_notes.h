// SPDX-License-Identifier: GPL-3.0-or-later
// One-line explanations for the Better Wind Waker options whose titles don't
// say what they do, shown under them in both desktop menus. The names are the
// option names in mods/betterww/options.txt.
#pragma once
#include <cstring>

inline const char* bw_option_note(const char* name) {
    if (name == nullptr)
        return nullptr;
    if (std::strcmp(name, "brisk_sail") == 0)
        return "Swift Sail with stronger brakes: the boat stops sooner when you let go. Turns on Swift Sail too.";
    if (std::strcmp(name, "unrestricted_boat") == 0)
        return "The King of Red Lions no longer turns you back before the story lets you sail somewhere. "
               "The edge of the map still does.";
    if (std::strcmp(name, "invert_camera_x") == 0)
        return "The game's own camera turns the other way: the keyboard's T F G H, and a controller when the "
               "fast right-stick camera is off. To invert a controller's camera, use Controls.";
    return nullptr;
}
