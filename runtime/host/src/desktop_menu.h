// SPDX-License-Identifier: GPL-3.0-or-later
// The desktop settings menu, one for Mac, Linux and Windows (desktop_menu.cpp):
// the settings as the host's own variables, the settings file, safe mode, the
// menu window and its tabs (drawn from settings_table.h through menu_draw.h),
// the hotkeys' toggles, the climbing wheel and the shader wait.
//
// What differs between the platforms is plumbing, behind the few calls in
// bw_platform below, which each platform's file defines: settings_menu.cpp on
// Mac and Linux, windows/src/win_settings.cpp on Windows. They also own their
// entry points: when the file is read, how keys reach the menu, the window.
#pragma once
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include "settings_table.h"

struct SDL_Window;
struct ImFont;

namespace bw_desktop {

using Lines = std::vector<std::pair<std::string, std::string>>;  // a settings file's KEY=VALUE lines

// --- the settings, as variables ---------------------------------------------

std::string env(const char* key, const char* fallback = "");
bool env_on(const char* key, bool fallback);
int env_int(const char* key, int fallback);
// A change the player made: the variable for this session, and the file.
void set_env(const char* key, const std::string& value);
// A default, where neither the file nor the launch said: never saved as an edit.
void env_default(const char* key, const std::string& value);
// Something only the platform keeps (the window's place) changed: save soon.
void mark_dirty();

// --- the file ---------------------------------------------------------------

// First, before anything sets a setting: what the launch (a launch option, or a
// variable set before BlueWake started) already chose wins for this session,
// and the file keeps its own value for it. `path` is the settings file ("" for
// none); `read_instead` is read when `path` does not exist yet (an older
// app's file), and the settings are still saved to `path`.
void load(const std::string& path, const std::string& read_instead = "");
// The launch sets a setting for this session as if it had been launched so
// (BLUEWAKE_STAGE_SELECT=1 arms the stage select's toggle); the file keeps
// its own value.
void launch_choice(const char* key, const std::string& value);
// The defaults this platform puts in the environment at launch (the table's
// fill), so the game reads them rather than its own. After safe mode.
void fill_defaults();
// Safe mode (a launch that never got to a clean exit, or --safe-mode): the
// file is copied aside, then this session and the file get the safe choices.
void apply_safe_mode();
bool safe_mode();
const std::string& settings_path();
// Saves, if anything changed.
void save();
// A change is still to be written (the last save failed, or none yet).
bool unsaved();
// The saves and session logs' folder, for the Config tab.
void set_data_dir(const std::string& dir);
const std::string& data_dir();

// --- the running menu --------------------------------------------------------

// Once Aurora is up and the launch has set everything: the overlay, the hold,
// and from here on what a restart would change.
void install();
void open_menu();
void close_menu();
bool menu_open();
// The stage select asked its question (stage_select_key.h): the menu shows it.
void ask_stage_select();
// The hotkeys: on or off at once, kept, and shown on the status line.
void toggle_hd_look();     // F2
void toggle_fps();         // F9
void toggle_smooth_motion(); // F10, at the rate last chosen
void toggle_fullscreen();  // F11 or Alt+Enter, for this session
SDL_Window* game_window();

}  // namespace bw_desktop

// What each platform defines for the shared menu.
namespace bw_platform {

// How the menu looks on this display: the scale of its sizes, the font scale
// for its windows, and a font to push (nullptr for ImGui's own).
struct Look {
    float scale = 1.f;
    float font_scale = 1.f;
    ImFont* font = nullptr;
};
// Every frame, before the menu draws: false while the platform still sets up
// (the first frame loads its font), and nothing is drawn that frame.
bool ready(SDL_Window* window);
Look look(SDL_Window* window);
// Every frame, after the shared overlays: the platform's own (Windows' hint,
// its window tracking and launch marker; Linux's renderer notice).
void frame(SDL_Window* window, unsigned long long now_ms);
// The menu opened or closed: what the game's input does meanwhile.
void menu_opened(bool open);
// The folder HD textures turn on with when none was chosen before; "" opens
// the folder picker instead.
std::string default_pack_folder();
// The app's Load/Textures folder, where Import from Wind Waker HD puts its pack.
std::string textures_folder();
void open_folder(const std::string& path);
void reset_window();
// Restart now: false where the app cannot start itself again (the Mac app).
bool can_restart();
void restart();

}  // namespace bw_platform
