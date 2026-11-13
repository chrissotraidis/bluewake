// SPDX-License-Identifier: GPL-3.0-or-later
// Dolphin saves, in the desktop menu's Config tab: Import Dolphin Save brings a
// quest log in from a Dolphin .gci or memory card (.raw), and Export Dolphin
// Save writes BlueWake's card out as a .gci, as the iPhone, iPad and Android
// apps' Game Data menus do, with the same code (apple/ios/src/dolphin_save_import.c).
// Both pickers open in the folder the last Dolphin save was picked from or
// saved to (BLUEWAKE_DOLPHIN_SAVE_FOLDER in the settings file).
#pragma once

struct SDL_Window;

namespace bw_dolphin_save {

// The section's buttons, or the import being chosen, with its last message.
void controls(SDL_Window* window);

}  // namespace bw_dolphin_save
