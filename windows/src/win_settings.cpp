// BlueWake for Windows: the settings, the in-game settings menu (F1 or Esc),
// the desktop hotkeys (F11 and Alt+Enter fullscreen, F10 Smooth Motion, F9 the
// frame rate) and the window's placement.
//
// The settings live in %APPDATA%\BlueWake\settings.ini. At launch they become
// the host's own variables (BLUEWAKE_*, DOL_*) unless the command line already
// set one, so the command line wins for that session. In the menu, display and
// control settings apply at once; the mods, Better Wind Waker's options, the
// picture's shape and the audio mode apply when BlueWake restarts (Restart now
// starts it again with the environment it began with).
//
// The menu is ImGui, drawn through the host overlay hook inside Aurora's frame
// (gxruntime/aurora_backend.h). The game keeps running under it; while it is
// open the keyboard is taken off the game's pad and the mouse camera stands
// aside, so the menu's clicks and keys do not move Link.
#include "win_settings.h"
#include "settings_state.h"
#include "smooth_rate.h"
#include "controller_face_swap.h"
#include "restart_request.h"
#include "launch_marker.h"
#include "atomic_file.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <SDL3/SDL.h>
#include <aurora/aurora.h>
#include <aurora/imgui.h>
#include <dolphin/pad.h>
#include <imgui.h>
#include "button_remap.h"
#include "input_remap.h"
#include "option_notes.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "gxruntime/aurora_backend.h"
#include "save_state.h"
#include "desktop_theme.h"
#include <aurora/gfx.h>

extern "C" {
// runtime/host/src/mouse_camera.h and game_options.h, declared here with plain
// types: those headers bring in the guest CPU's, which this file has no use for.
void bluewake_mouse_camera_configure(bool enabled, double sensitivity, bool invert_y);
void bluewake_mouse_camera_block(bool blocked);
bool bluewake_mouse_camera_captured(void);
void bluewake_mouse_camera_buttons(const char* map);
void bluewake_haptics_reload(void);
void bluewake_mouse_camera_reload(void);
void bluewake_climb_reload(void);
// forest_water.h: BLUEWAKE_FOREST_WATER_KEEP_TREES and _30_MINUTES read again.
void bluewake_forest_water_reload(void);
void bluewake_haptics_block(bool blocked);
const char* bluewake_game_options_describe(uint32_t position, const char** title, bool* default_on, bool* on);
// climb.h: the stamina wheel's state for the HUD.
bool bluewake_climb_hud(float* fraction, bool* exhausted, float* x, float* y, float* aspect, float* alpha);
}

// Aurora's frame counters (lib/gfx/common.hpp, linked in statically), for the
// session log: every present, and the game's own frames.
namespace aurora::gfx {
float calculate_fps() noexcept;
float calculate_game_fps() noexcept;
}  // namespace aurora::gfx

namespace {

Settings g_saved;     // as in the file, changed by the menu and hotkeys
Settings g_session, g_before_edit;
Settings g_launched;  // as this session started (what a restart would change)
std::string g_data_dir, g_path;
std::vector<wchar_t> g_environment;  // as BlueWake was started, for Restart

RestartRequest g_restart;
bool g_safe_mode;
bool g_menu_open;
bool g_toggle_menu, g_toggle_fullscreen;  // from the hotkeys, done in the frame
bool g_dirty;
Uint64 g_dirty_at, g_first_frame_at;
bool g_placed;
bool g_pad_applied;
SDL_JoystickID g_pad_connection = 0;
float g_font_scale = 1.0f;  // the scale the UI font was drawn at (see load_font)

const char* const kScaleNames[] = {"The window's own pixels", "1x (640x480)", "2x (1280x960)",
                                   "3x (1920x1440)", "4x (2560x1920)"};
const int kAnisotropy[] = {1, 2, 4, 8, 16};
const char* const kAnisotropyNames[] = {"The game's own", "2x anisotropic", "4x anisotropic", "8x anisotropic",
                                        "16x anisotropic"};

// --- the file ---------------------------------------------------------------

bool parse_bool(const std::string& v) { return v == "1" || v == "true" || v == "on" || v == "yes"; }

void load_file() {
    FILE* f = std::fopen(g_path.c_str(), "r");
    if (f == nullptr)
        return;
    char line[512];
    while (std::fgets(line, sizeof line, f) != nullptr) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
            s.pop_back();
        const size_t eq = s.find('=');
        if (s.empty() || s[0] == '#' || eq == std::string::npos)
            continue;
        const std::string k = s.substr(0, eq), v = s.substr(eq + 1);
        Settings& d = g_saved;
        if (k == "fullscreen") d.fullscreen = parse_bool(v);
        else if (k == "window") std::sscanf(v.c_str(), "%dx%d", &d.window_w, &d.window_h);
        else if (k == "window_position") std::sscanf(v.c_str(), "%d,%d", &d.window_x, &d.window_y);
        else if (k == "render_scale") d.render_scale = std::clamp(std::atoi(v.c_str()), 0, 4);
        else if (k == "anisotropy") d.anisotropy = std::clamp(std::atoi(v.c_str()), 1, 16);
        else if (k == "smooth_motion") d.smooth_motion = parse_bool(v);
        else if (k == "smooth_motion_fps") d.smooth_steps = v == "display" ? -1 : std::atoi(v.c_str()) >= 120 ? 3 : 1;
        else if (k == "show_fps") d.show_fps = parse_bool(v);
        else if (k == "pause_unfocused") d.pause_unfocused = parse_bool(v);
        else if (k == "compile_shaders_first") d.shaders_first = parse_bool(v);
        else if (k == "mouse_camera") d.mouse_camera = parse_bool(v);
        else if (k == "mouse_sensitivity") d.mouse_sensitivity = std::clamp(std::atof(v.c_str()), 0.1, 10.0);
        else if (k == "mouse_invert_y") d.mouse_invert_y = parse_bool(v);
        else if (k == "controller_swap_ab") d.controller_swap_ab = parse_bool(v);
        else if (k == "button_map") d.button_map = v;
        else if (k == "mouse_buttons") d.mouse_buttons = v;
        else if (k == "key_map") d.key_map = v;
        else if (k == "controller_swap_xy") d.controller_swap_xy = parse_bool(v);
        else if (k == "controller_invert_x") d.pad_invert_x = parse_bool(v);
        else if (k == "controller_invert_y") d.pad_invert_y = parse_bool(v);
        else if (k == "stick_camera") d.stick_camera = parse_bool(v);
        else if (k == "stick_camera_speed") d.stick_speed = std::clamp(std::atoi(v.c_str()), 60, 1080);
        else if (k == "stick_aim_speed") d.stick_aim_speed = std::clamp(std::atoi(v.c_str()), 30, 720);
        else if (k == "aim_invert_y") d.aim_invert_y = parse_bool(v);
        else if (k == "climb") d.climb = parse_bool(v);
        else if (k == "climb_stamina") d.climb_stamina = std::clamp(std::atoi(v.c_str()), 4, 30);
        else if (k == "forest_water_keep_trees") d.forest_keep_trees = parse_bool(v);
        else if (k == "forest_water_30_minutes") d.forest_30_minutes = parse_bool(v);
        else if (k == "haptics") d.haptics = v == "off" ? 0 : v == "classic" ? 1 : 2;
        else if (k == "haptics_strength") d.haptics_strength = std::clamp(std::atoi(v.c_str()), 0, 100);
        else if (k == "haptics_triggers") d.haptics_triggers = parse_bool(v);
        else if (k == "aspect") d.aspect = (v == "16:9" || v == "16:10") ? v : "4:3";
        else if (k == "keep_aspect") d.keep_aspect = parse_bool(v);
        else if (k == "betterww") d.betterww = parse_bool(v);
        else if (k.rfind("option.", 0) == 0) d.options[k.substr(7)] = parse_bool(v);
        else if (k == "hd_textures") d.hd_textures = parse_bool(v);
        else if (k == "texture_pack") d.texture_pack = v;
        else if (k == "lle_audio") d.lle_audio = parse_bool(v);
        else if (k == "movement_extras") d.movement_extras = parse_bool(v);
        else if (k == "fast_transitions") d.fast_transitions = parse_bool(v);
        else if (k == "quick_doors") d.quick_doors = parse_bool(v);
    }
    std::fclose(f);
}

void save_file() {
    g_dirty_at = SDL_GetTicks(); // back off after failures rather than retry every frame
    char* pending = bw_atomic_path(g_path.c_str());
    FILE* f = pending != nullptr ? std::fopen(pending, "w") : nullptr;
    if (f == nullptr) { free(pending); return; }
    const Settings& d = g_saved;
    std::fprintf(f, "# BlueWake settings (the in-game menu, F1, writes this file)\n");
    std::fprintf(f, "fullscreen=%d\n", d.fullscreen);
    if (d.window_w > 0 && d.window_h > 0)
        std::fprintf(f, "window=%dx%d\n", d.window_w, d.window_h);
    if (d.window_x != INT_MIN && d.window_y != INT_MIN)
        std::fprintf(f, "window_position=%d,%d\n", d.window_x, d.window_y);
    std::fprintf(f, "render_scale=%d\nanisotropy=%d\nsmooth_motion=%d\nshow_fps=%d\npause_unfocused=%d\n",
                 d.render_scale, d.anisotropy, d.smooth_motion, d.show_fps, d.pause_unfocused);
    std::fprintf(f, "compile_shaders_first=%d\n", d.shaders_first);
    std::fprintf(f, "smooth_motion_fps=%s\n", d.smooth_steps == -1 ? "display" : d.smooth_steps >= 3 ? "120" : "60");
    std::fprintf(f, "mouse_camera=%d\nmouse_sensitivity=%.2f\nmouse_invert_y=%d\n", d.mouse_camera,
                 d.mouse_sensitivity, d.mouse_invert_y);
    std::fprintf(f, "controller_swap_ab=%d\ncontroller_swap_xy=%d\n", d.controller_swap_ab, d.controller_swap_xy);
    if (!d.button_map.empty())
        std::fprintf(f, "button_map=%s\n", d.button_map.c_str());
    if (!d.mouse_buttons.empty())
        std::fprintf(f, "mouse_buttons=%s\n", d.mouse_buttons.c_str());
    if (!d.key_map.empty())
        std::fprintf(f, "key_map=%s\n", d.key_map.c_str());
    std::fprintf(f, "controller_invert_x=%d\ncontroller_invert_y=%d\n", d.pad_invert_x, d.pad_invert_y);
    std::fprintf(f, "stick_camera=%d\nstick_camera_speed=%d\nstick_aim_speed=%d\n", d.stick_camera, d.stick_speed,
                 d.stick_aim_speed);
    std::fprintf(f, "aim_invert_y=%d\n", d.aim_invert_y);
    std::fprintf(f, "climb=%d\nclimb_stamina=%d\n", d.climb, d.climb_stamina);
    std::fprintf(f, "forest_water_keep_trees=%d\nforest_water_30_minutes=%d\n", d.forest_keep_trees,
                 d.forest_30_minutes);
    if (!d.texture_pack.empty())
        std::fprintf(f, "texture_pack=%s\n", d.texture_pack.c_str());
    std::fprintf(f, "haptics=%s\nhaptics_strength=%d\nhaptics_triggers=%d\n",
                 d.haptics == 0 ? "off" : d.haptics == 1 ? "classic" : "enhanced",
                 d.haptics_strength, d.haptics_triggers);
    std::fprintf(f, "aspect=%s\nkeep_aspect=%d\nbetterww=%d\nhd_textures=%d\nlle_audio=%d\n", d.aspect.c_str(),
                 d.keep_aspect, d.betterww, d.hd_textures, d.lle_audio);
    for (const auto& [name, on] : d.options)
        std::fprintf(f, "option.%s=%d\n", name.c_str(), on);
    std::fprintf(f, "movement_extras=%d\nfast_transitions=%d\nquick_doors=%d\n",
                 d.movement_extras, d.fast_transitions, d.quick_doors);
    const bool ok = bw_atomic_finish_dirty(f, pending, g_path.c_str(), &g_dirty);
    free(pending);
    if (!ok) std::fprintf(stderr, "[settings] save failed; previous file kept, retry pending\n");
}

void changed() {
    bw_settings_keep_edits(g_saved, g_before_edit, g_session);
    g_before_edit = g_session;
    g_dirty = true;
    g_dirty_at = SDL_GetTicks();
}

// --- the launch -------------------------------------------------------------

bool env_set(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0';
}

void env_default(const char* name, const std::string& value) {
    if (!env_set(name))
        _putenv_s(name, value.c_str());
}

double aspect_ratio(const std::string& aspect) {
    return aspect == "16:9" ? 16.0 / 9.0 : aspect == "16:10" ? 1.6 : 4.0 / 3.0;
}

// A window a good size for this screen: the tallest multiple of 240 lines that
// fits in 80 percent of the primary screen's work area, at the picture's shape.
void default_window(double ratio, int* w, int* h) {
    MONITORINFO info{};
    info.cbSize = sizeof info;
    int work_w = 1280, work_h = 1024;
    if (GetMonitorInfoW(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY), &info)) {
        work_w = info.rcWork.right - info.rcWork.left;
        work_h = info.rcWork.bottom - info.rcWork.top;
    }
    int height = std::max(480, static_cast<int>(work_h * 0.8) / 240 * 240);
    int width = static_cast<int>(std::lround(height * ratio));
    if (width > work_w * 9 / 10) {
        width = work_w * 9 / 10;
        height = static_cast<int>(width / ratio);
    }
    *w = width;
    *h = height;
}

std::string texture_folder() { return g_data_dir + "Load\\Textures\\GZLE01"; }
// The folder the pack is loaded from: the one chosen, else texture_folder().
std::string texture_pack_folder(const Settings& d) {
    return d.texture_pack.empty() ? texture_folder() : d.texture_pack;
}

// --- the window -------------------------------------------------------------

SDL_Window* game_window() {
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);
    SDL_Window* window = windows != nullptr && count > 0 ? windows[0] : nullptr;
    SDL_free(windows);
    return window;
}

bool is_fullscreen(SDL_Window* w) { return (SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN) != 0; }

void set_fullscreen(SDL_Window* w, bool on) {
    if (w == nullptr)
        return;
    SDL_SetWindowFullscreen(w, on);
    g_session.fullscreen = on;
    changed();
    std::fprintf(stderr, "[windows] fullscreen %s\n", on ? "on" : "off");
}

// Once the window exists: where it was last time, if that is still on a
// screen with its title bar showing, else centred. Aurora would put its client
// area at the screen's corner, the title bar above the top edge.
void place_window(SDL_Window* w) {
    if (is_fullscreen(w))
        return;
    const Settings& d = g_session;
    bool restored = false;
    if (d.window_x != INT_MIN && d.window_y != INT_MIN) {
        const SDL_Point title{d.window_x + 60, d.window_y - 16};
        const SDL_Point corner{d.window_x + 60, d.window_y + 60};
        if (SDL_GetDisplayForPoint(&title) != 0 && SDL_GetDisplayForPoint(&corner) != 0) {
            SDL_SetWindowPosition(w, d.window_x, d.window_y);
            restored = true;
        }
    }
    if (!restored)
        SDL_SetWindowPosition(w, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

// Remember the window's place and size as the player leaves them.
void track_window(SDL_Window* w) {
    const SDL_WindowFlags flags = SDL_GetWindowFlags(w);
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED | SDL_WINDOW_MAXIMIZED))
        return;
    int x = 0, y = 0, width = 0, height = 0;
    if (!SDL_GetWindowPosition(w, &x, &y) || !SDL_GetWindowSize(w, &width, &height))
        return;
    Settings& d = g_session;
    static bool first = true;
    if (first) {
        first = false;
        d.window_x = g_before_edit.window_x = x;
        d.window_y = g_before_edit.window_y = y;
        d.window_w = g_before_edit.window_w = width;
        d.window_h = g_before_edit.window_h = height;
        return;
    }
    if (x != d.window_x || y != d.window_y || width != d.window_w || height != d.window_h) {
        d.window_x = x;
        d.window_y = y;
        d.window_w = width;
        d.window_h = height;
        changed();
    }
}

void reset_window(SDL_Window* w) {
    if (w == nullptr)
        return;
    if (is_fullscreen(w))
        set_fullscreen(w, false);
    int width = 0, height = 0;
    default_window(aspect_ratio(g_launched.aspect), &width, &height);
    SDL_SetWindowSize(w, width, height);
    SDL_SetWindowPosition(w, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

// --- settings that apply at once ------------------------------------------------

// A controller's camera stick, inverted as asked, as the iOS app does it
// (apple/ios/src/controller_apply.cpp). Only touched once the player inverts
// an axis, so a mapping set elsewhere is otherwise left alone.
void apply_controller() {
    const Settings& d = g_session;
    BwButtonMap map;
    const bool remapped = bw_button_map_parse(d.button_map, &map);
    if (!d.pad_invert_x && !d.pad_invert_y && !d.controller_swap_ab && !d.controller_swap_xy && !remapped &&
        !g_pad_applied)
        return;
    if (PADGetIndexForPort(0) < 0)
        return;
    PADRestoreDefaultMapping(0);
    // A custom layout replaces the swaps; otherwise the swaps as before.
    if (remapped)
        bw_apply_button_map(0, map);
    else
        bw_apply_face_swaps(0, d.controller_swap_ab, d.controller_swap_xy);
    bw_apply_camera_axes(0, d.pad_invert_x, d.pad_invert_y);
    g_pad_applied = true;
}

// The keyboard's keys for the GameCube buttons (input_remap.h), applied once
// the pad's keyboard bindings exist and again whenever they change.
void apply_key_map() {
    static bool applied = false;
    static std::string last;
    const std::string& text = g_session.key_map;
    if (applied && text == last)
        return;
    BwKeyMap map;
    bw_key_map_parse(text, &map);
    if (bw_apply_key_map(0, map)) {
        applied = true;
        last = text;
    }
}

// Adapted from Elliott Tate's display-rate selection; resolve the current
// session so a command-line override never changes a saved preference.
float display_refresh(SDL_Window* window) {
    const SDL_DisplayMode* mode = window != nullptr ? SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window)) : nullptr;
    return mode != nullptr ? mode->refresh_rate : 0.f;
}

void apply_smooth_rate(SDL_Window* window) {
    aurora_set_frame_interp_steps(bw_smooth_steps(g_session.smooth_steps, display_refresh(window)));
}

const char* haptics_name(int mode) { return mode == 0 ? "off" : mode == 1 ? "classic" : "enhanced"; }

void apply_haptics() {
    const Settings& d = g_session;
    _putenv_s("BLUEWAKE_HAPTICS", haptics_name(d.haptics));
    _putenv_s("BLUEWAKE_HAPTICS_STRENGTH", std::to_string(d.haptics_strength).c_str());
    _putenv_s("BLUEWAKE_HAPTICS_TRIGGERS", d.haptics_triggers ? "1" : "0");
    bluewake_haptics_reload();
}

// The fast right-stick camera reads the controller through SDL itself, so the
// controller's inversion (apply_controller, for the game's own C-stick) is its
// BLUEWAKE_STICK_CAMERA_INVERT_X and _Y too (as in Elliott Tate's Windows menu).
void apply_stick() {
    const Settings& d = g_session;
    _putenv_s("BLUEWAKE_STICK_CAMERA", d.stick_camera ? "1" : "0");
    _putenv_s("BLUEWAKE_STICK_CAMERA_SPEED", std::to_string(d.stick_speed).c_str());
    _putenv_s("BLUEWAKE_STICK_AIM_SPEED", std::to_string(d.stick_aim_speed).c_str());
    _putenv_s("BLUEWAKE_STICK_CAMERA_INVERT_X", d.pad_invert_x ? "1" : "0");
    _putenv_s("BLUEWAKE_STICK_CAMERA_INVERT_Y", d.pad_invert_y ? "1" : "0");
    _putenv_s("BLUEWAKE_AIM_INVERT_Y", d.aim_invert_y ? "1" : "0");
    bluewake_mouse_camera_reload();
}

void apply_climb() {
    const Settings& d = g_session;
    _putenv_s("BLUEWAKE_CLIMB", d.climb ? "1" : "0");
    _putenv_s("BLUEWAKE_CLIMB_STAMINA", std::to_string(d.climb_stamina).c_str());
    bluewake_climb_reload();
}

void apply_forest_water() {
    const Settings& d = g_session;
    _putenv_s("BLUEWAKE_FOREST_WATER_KEEP_TREES", d.forest_keep_trees ? "1" : "0");
    _putenv_s("BLUEWAKE_FOREST_WATER_30_MINUTES", d.forest_30_minutes ? "1" : "0");
    bluewake_forest_water_reload();
}

void apply_live() {
    const Settings& d = g_session;
    aurora_set_frame_buffer_scale(static_cast<float>(d.render_scale));
    aurora_set_forced_anisotropy(static_cast<unsigned>(d.anisotropy));
    apply_smooth_rate(game_window());
    aurora_set_frame_interpolation(d.smooth_motion);
    aurora_set_fps_overlay(d.show_fps);
    aurora_set_pause_on_focus_lost(d.pause_unfocused);
    bluewake_mouse_camera_configure(d.mouse_camera, d.mouse_sensitivity, d.mouse_invert_y);
    bluewake_mouse_camera_buttons(d.mouse_buttons.c_str());
    apply_controller();
    apply_stick();
    apply_climb();
    apply_forest_water();
}

void set_menu_open(bool open) {
    if (open == g_menu_open)
        return;
    g_menu_open = open;
    PADBlockInput(open);
    // The menu's keys and clicks are not the game's.
    PADSetKeyboardActive(0, open ? FALSE : TRUE);
    bluewake_mouse_camera_block(open);
    bluewake_haptics_block(open);
    std::fprintf(stderr, "[windows] settings menu %s\n", open ? "open" : "closed");
    if (!open && g_dirty)
        save_file();
}

void open_folder(const std::string& path) {
    CreateDirectoryA(path.c_str(), nullptr);
    wchar_t wide[MAX_PATH * 2];
    if (MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide, MAX_PATH * 2) > 0)
        ShellExecuteW(nullptr, L"open", wide, nullptr, nullptr, SW_SHOWNORMAL);
}

void restart() {
    if (g_dirty) save_file();
    bool ok = g_restart.request(!g_dirty, [] {
        SDL_Event quit{};
        quit.type = SDL_EVENT_QUIT;
        return SDL_PushEvent(&quit);
    });
    if (!ok) std::fprintf(stderr, "[windows] restart refused: settings save or quit request failed\n");
}

// Start BlueWake again, with the command line and environment it began with,
// so the settings file decides what the new session is.
bool relaunch() {
    wchar_t exe[MAX_PATH * 2];
    if (GetModuleFileNameW(nullptr, exe, MAX_PATH * 2) == 0)
        return false;
    std::vector<wchar_t> command(GetCommandLineW(), GetCommandLineW() + wcslen(GetCommandLineW()) + 1);
    STARTUPINFOW startup{};
    startup.cb = sizeof startup;
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe, command.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                        g_environment.empty() ? nullptr : g_environment.data(), nullptr, &startup, &process)) {
        std::fprintf(stderr, "[windows] restart failed (error %lu)\n", GetLastError());
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    std::fprintf(stderr, "[windows] restarting with the new settings\n");
    return true;
}

// --- the menu ---------------------------------------------------------------

bool needs_restart() {
    const Settings &a = g_session, &b = g_launched;
    return a.aspect != b.aspect || a.keep_aspect != b.keep_aspect || a.betterww != b.betterww ||
           a.options != b.options || a.hd_textures != b.hd_textures || a.texture_pack != b.texture_pack ||
           a.lle_audio != b.lle_audio ||
           a.movement_extras != b.movement_extras || a.fast_transitions != b.fast_transitions ||
           a.quick_doors != b.quick_doors;
}

void restart_note(bool differs) {
    if (differs) {
        ImGui::SameLine();
        ImGui::TextDisabled("*");
    }
}

void tab_display(SDL_Window* w) {
    Settings& d = g_session;
    bool full = w != nullptr && is_fullscreen(w);
    if (ImGui::Checkbox("Fullscreen   (F11 or Alt+Enter)", &full))
        set_fullscreen(w, full);
    if (ImGui::Checkbox("Smooth Motion (experimental)   (F10)", &d.smooth_motion)) {
        apply_smooth_rate(w);
        aurora_set_frame_interpolation(d.smooth_motion);
        changed();
    }
    static const char* const rates[] = {"60 FPS", "120 FPS", "Match the display (up to 240 FPS)"};
    int rate = d.smooth_steps == -1 ? 2 : d.smooth_steps >= 3 ? 1 : 0;
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 23);
    if (ImGui::Combo("Smooth Motion rate", &rate, rates, IM_ARRAYSIZE(rates))) {
        d.smooth_steps = rate == 2 ? -1 : rate == 1 ? 3 : 1;
        apply_smooth_rate(w);
        changed();
    }
    const float refresh = display_refresh(w);
    ImGui::TextDisabled("    Target: %d FPS on this %.0f Hz display; pacing may reduce it.",
                        d.smooth_motion ? (bw_smooth_steps(d.smooth_steps, refresh) + 1) * 30 : 30, refresh);
    ImGui::TextDisabled("    The game itself still runs at 30. Experimental: off by default.");
    if (ImGui::Checkbox("Show the frame rate   (F9)", &d.show_fps)) {
        aurora_set_fps_overlay(d.show_fps);
        changed();
    }
    ImGui::Spacing();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
    if (ImGui::Combo("Render resolution", &d.render_scale, kScaleNames, IM_ARRAYSIZE(kScaleNames))) {
        aurora_set_frame_buffer_scale(static_cast<float>(d.render_scale));
        changed();
    }
    int filtering = 0;
    for (int i = 0; i < IM_ARRAYSIZE(kAnisotropy); i++)
        if (kAnisotropy[i] == d.anisotropy)
            filtering = i;
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
    if (ImGui::Combo("Texture filtering", &filtering, kAnisotropyNames, IM_ARRAYSIZE(kAnisotropyNames))) {
        d.anisotropy = kAnisotropy[filtering];
        aurora_set_forced_anisotropy(static_cast<unsigned>(d.anisotropy));
        changed();
    }
    if (ImGui::Checkbox("Keep the picture's shape (black bars rather than stretching)", &d.keep_aspect))
        changed();
    restart_note(d.keep_aspect != g_launched.keep_aspect);
    if (ImGui::Checkbox("Pause while the window is in the background", &d.pause_unfocused)) {
        aurora_set_pause_on_focus_lost(d.pause_unfocused);
        changed();
    }
    if (ImGui::Checkbox("Compile shaders before playing", &d.shaders_first))
        changed();
    restart_note(d.shaders_first != g_launched.shaders_first);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("At start, wait until the shaders from earlier play are ready, so nothing is missing "
                          "from the picture the first time it is drawn. Otherwise they are made while you play.");
    ImGui::Spacing();
    if (ImGui::Button("Reset the window"))
        reset_window(w);
    ImGui::SameLine();
    ImGui::TextDisabled("Drag the title bar to move it; drag an edge to size it.");
}

void tab_controls() {
    Settings& d = g_session;
    bool mouse = false;
    if (ImGui::Checkbox("Mouse camera: click the game, then move the mouse (Esc gives it back)", &d.mouse_camera))
        mouse = true;
    float sensitivity = static_cast<float>(d.mouse_sensitivity);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
    if (ImGui::SliderFloat("Mouse sensitivity", &sensitivity, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
        d.mouse_sensitivity = sensitivity;
        mouse = true;
    }
    if (ImGui::Checkbox("Mouse forward looks down", &d.mouse_invert_y))
        mouse = true;
    if (ImGui::CollapsingHeader("Mouse buttons")) {
        BwMouseMap buttons;
        bw_mouse_map_parse(d.mouse_buttons.c_str(), &buttons);
        ImGui::TextWrapped("What each mouse button presses while the mouse is the camera. The first left click "
                           "only hands the mouse to the game.");
        if (bw_mouse_map_ui(&buttons)) {
            char text[64];
            bw_mouse_map_format(&buttons, text, sizeof text);
            d.mouse_buttons = text;
            bluewake_mouse_camera_buttons(d.mouse_buttons.c_str());
            changed();
        }
    }
    if (mouse) {
        bluewake_mouse_camera_configure(d.mouse_camera, d.mouse_sensitivity, d.mouse_invert_y);
        changed();
    }
    ImGui::Spacing();
    bool stick = ImGui::Checkbox("Fast right-stick camera and aiming (like the mouse; click the stick for first person)",
                                 &d.stick_camera);
    ImGui::BeginDisabled(!d.stick_camera);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
    stick |= ImGui::SliderInt("Right-stick turn speed", &d.stick_speed, 120, 720, "%d degrees a second");
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
    stick |= ImGui::SliderInt("Right-stick aim speed (first person, items)", &d.stick_aim_speed, 60, 480,
                              "%d degrees a second");
    ImGui::EndDisabled();
    bool pad = ImGui::Checkbox("Controller: camera stick left and right inverted", &d.pad_invert_x);
    pad |= ImGui::Checkbox("Controller: camera stick up and down inverted", &d.pad_invert_y);
    stick |= ImGui::Checkbox("Left stick up and down inverted when aiming (first person, items)", &d.aim_invert_y);
    BwButtonMap map;
    const bool remapped = bw_button_map_parse(d.button_map, &map);
    ImGui::BeginDisabled(remapped);
    pad |= ImGui::Checkbox("Swap A and B", &d.controller_swap_ab);
    pad |= ImGui::Checkbox("Swap X and Y", &d.controller_swap_xy);
    ImGui::EndDisabled();
    if (ImGui::CollapsingHeader("Controller buttons")) {
        ImGui::TextWrapped("Choose which controller button presses each GameCube button. Picking one that is "
                           "already used swaps the two.%s", remapped ? " The swaps above are off while you use "
                           "a custom layout." : "");
        if (bw_button_map_ui(&map)) {
            d.button_map = bw_button_map_format(map);
            pad = true;
        }
    }
    if (pad || stick) {
        apply_controller();
        apply_stick();
        changed();
    }
    ImGui::Spacing();
    ImGui::SeparatorText("Haptics");
    // The game's vibration on an Xbox controller, a DualSense and others:
    // rendered from what the game asked for, or its own on-off motor.
    static const char* const kHaptics[] = {"Off", "Classic (the game's own on and off)",
                                           "Enhanced (shaped, with the triggers)"};
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
    bool haptics = ImGui::Combo("Controller vibration", &d.haptics, kHaptics, IM_ARRAYSIZE(kHaptics));
    ImGui::BeginDisabled(d.haptics != 2);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16);
    haptics |= ImGui::SliderInt("Vibration strength", &d.haptics_strength, 0, 100, "%d%%");
    ImGui::EndDisabled();
    ImGui::BeginDisabled(d.haptics != 2);
    haptics |= ImGui::Checkbox("Trigger feedback (Xbox impulse triggers, DualSense trigger vibration)",
                               &d.haptics_triggers);
    ImGui::EndDisabled();
    ImGui::TextDisabled(d.haptics == 2   ? "    Hits, falls, explosions and quakes as the game times them, shaped by their strength."
                        : d.haptics == 1 ? "    The motor on and off, as a GameCube controller's."
                                         : "    No vibration. (The game's own Vibration option turns it off too.)");
    if (haptics) {
        apply_haptics();
        changed();
    }
    ImGui::Spacing();
    ImGui::SeparatorText("Keyboard");
    BwKeyMap keys;
    bw_key_map_parse(d.key_map, &keys);
    if (ImGui::CollapsingHeader("Keyboard keys for the GameCube buttons")) {
        ImGui::TextWrapped("Choose the key for each GameCube button. Picking one that is already used swaps the two.");
        if (bw_key_map_ui(&keys)) {
            d.key_map = bw_key_map_format(keys);
            changed();
        }
    }
    char face[96], shoulder[96];
    std::snprintf(face, sizeof face, "%s  %s  %s  %s", bw_key_name(keys.scancode[0]), bw_key_name(keys.scancode[1]),
                  bw_key_name(keys.scancode[2]), bw_key_name(keys.scancode[3]));
    std::snprintf(shoulder, sizeof shoulder, "%s  %s  %s", bw_key_name(keys.scancode[5]),
                  bw_key_name(keys.scancode[6]), bw_key_name(keys.scancode[4]));
    if (ImGui::BeginTable("keys", 2, ImGuiTableFlags_SizingFixedFit)) {
        const char* const rows[][2] = {
            {"Control stick", "W A S D"}, {"C-stick", "T F G H"}, {"D-pad", "Arrow keys"},
            {"A  B  X  Y", face}, {"L  R  Z", shoulder}, {"START", bw_key_name(keys.scancode[7])},
            {"Jump", "Space (controller: left bumper)"}, {"Sprint", "Shift (controller: click the left stick)"},
            {"Camera zoom", "Mouse wheel, while the mouse is the camera"},
            {"Settings", "F1 or Esc"}, {"Fullscreen", "F11 or Alt+Enter"}, {"Smooth Motion", "F10"},
            {"Frame rate", "F9"},
        };
        for (const auto& row : rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(row[0]);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", row[1]);
        }
        ImGui::EndTable();
    }
    ImGui::TextDisabled("Game controllers work as they are plugged in (Xbox, PlayStation, Switch Pro, ...).");
}

void tab_enhancements() {
    Settings& d = g_session;
    ImGui::SeparatorText("Gameplay (after restart)");
    if (ImGui::Checkbox("Jump and Run", &d.movement_extras))
        changed();
    restart_note(d.movement_extras != g_launched.movement_extras);
    if (ImGui::Checkbox("Fast transitions", &d.fast_transitions))
        changed();
    restart_note(d.fast_transitions != g_launched.fast_transitions);
    if (ImGui::Checkbox("Quick doors", &d.quick_doors))
        changed();
    restart_note(d.quick_doors != g_launched.quick_doors);
    if (ImGui::Checkbox("Climb any wall (experimental)", &d.climb)) {
        apply_climb();
        changed();
    }
    ImGui::BeginDisabled(!d.climb);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
    if (ImGui::SliderInt("Climbing stamina", &d.climb_stamina, 4, 30, "%d seconds")) {
        apply_climb();
        changed();
    }
    ImGui::EndDisabled();
    if (ImGui::Checkbox("Forest Water: keep watered trees when time runs out", &d.forest_keep_trees)) {
        apply_forest_water();
        changed();
    }
    ImGui::TextDisabled("    Forest Water still expires. Refill and carry on with the trees still to water.");
    if (ImGui::Checkbox("Forest Water: 30-minute timer", &d.forest_30_minutes)) {
        apply_forest_water();
        changed();
    }
    ImGui::TextDisabled("    Applies the next time Link scoops Forest Water.");
    ImGui::Spacing();
    ImGui::TextUnformatted("Picture");
    restart_note(d.aspect != g_launched.aspect);
    const char* const aspects[][2] = {{"4:3", "4:3, the game's own"},
                                      {"16:10", "16:10 widescreen"},
                                      {"16:9", "16:9 widescreen"}};
    for (const auto& a : aspects) {
        if (ImGui::RadioButton(a[1], d.aspect == a[0])) {
            d.aspect = a[0];
            changed();
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();
    ImGui::TextDisabled("    Widescreen widens the camera and moves the HUD to the edges.");
    ImGui::Spacing();
    if (ImGui::Checkbox("Better Wind Waker", &d.betterww))
        changed();
    restart_note(d.betterww != g_launched.betterww);
    ImGui::TextDisabled("    Wind Waker HD's changes. Each can be turned on or off:");
    ImGui::BeginDisabled(!d.betterww);
    ImGui::Indent();
    bool any = false;
    for (uint32_t i = 0;; i++) {
        const char* title = nullptr;
        bool default_on = false, on = false;
        const char* name = bluewake_game_options_describe(i, &title, &default_on, &on);
        if (name == nullptr)
            break;
        any = true;
        bool value = bw_settings_option_value(d, name, default_on);
        ImGui::PushID(name);
        if (ImGui::Checkbox(title != nullptr ? title : name, &value)) {
            d.options[name] = value;
            changed();
        }
        const bool was = bw_settings_option_value(g_launched, name, default_on);
        restart_note(value != was);
        if (const char* note = bw_option_note(name)) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
            ImGui::TextDisabled("%s", note);
            ImGui::PopTextWrapPos();
        }
        ImGui::PopID();
    }
    if (!any)
        ImGui::TextDisabled("This build has no Better Wind Waker options (build with mods).");
    ImGui::Unindent();
    ImGui::EndDisabled();
    ImGui::Spacing();
    if (ImGui::Checkbox("HD texture pack", &d.hd_textures))
        changed();
    restart_note(d.hd_textures != g_launched.hd_textures);
    ImGui::SameLine();
    if (ImGui::SmallButton("Open the texture folder"))
        open_folder(texture_pack_folder(d));
    if (d.texture_pack.empty()) {
        ImGui::TextDisabled("    A Dolphin-format pack for GZLE01 (its folder of .png or .dds files) goes in that folder.");
    } else {
        ImGui::TextDisabled("    The pack in %s", d.texture_pack.c_str());
        restart_note(d.texture_pack != g_launched.texture_pack);
        ImGui::Indent();
        if (ImGui::SmallButton("Use Load\\Textures\\GZLE01 instead")) {
            d.texture_pack.clear();
            changed();
        }
        ImGui::Unindent();
    }
}

void tab_game() {
    Settings& d = g_session;
    ImGui::TextUnformatted("Experimental debug save states");
    if (ImGui::Button("Save state (F6)")) {
        bluewake_save_state_hotkey(false);
        set_menu_open(false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load latest state (F8)")) {
        bluewake_save_state_hotkey(true);
        set_menu_open(false);
    }
    ImGui::TextDisabled("Keep normal memory-card saves. States depend on this game translation.");
    ImGui::Spacing();
    ImGui::TextUnformatted("Sound");
    restart_note(d.lle_audio != g_launched.lle_audio);
    if (ImGui::RadioButton("Fast (Dolphin's high-level Zelda sound)", !d.lle_audio)) {
        d.lle_audio = false;
        changed();
    }
    if (ImGui::RadioButton("Exact (the sound chip's own program; slower)", d.lle_audio)) {
        d.lle_audio = true;
        changed();
    }
    ImGui::Spacing();
    ImGui::SeparatorText("Your files");
    ImGui::TextDisabled("Saves, settings and logs are kept apart from the build:");
    ImGui::TextUnformatted(g_data_dir.c_str());
    if (ImGui::Button("Open that folder"))
        open_folder(g_data_dir);
    ImGui::SameLine();
    if (ImGui::Button("Open the session logs"))
        open_folder(g_data_dir + "logs");
}

// The UI's size for this window: the display's scale (Windows' own, or more
// on a big screen at 100 percent), smaller when the window is too small for
// the menu. No larger than the font was drawn at.
float ui_scale(SDL_Window* w) {
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    const float display = w != nullptr ? SDL_GetWindowDisplayScale(w) : 1.0f;
    const float wanted = std::max({1.0f, display, size.y / 900.0f});
    const float fits = std::min(size.x / 660.0f, size.y / 560.0f);
    return std::clamp(std::min(wanted, fits), 0.75f, g_font_scale);
}

// On the first frame: a clear font drawn at the screen's scale, the default
// for everything ImGui draws from the next frame on (the menu and Aurora's
// frame rate). ImGui's own is a 13-pixel bitmap font, tiny on a 4K screen and
// blurred when scaled.
//
// The font has an atlas of its own, handed to Aurora as an ImGui texture
// (aurora_imgui_add_texture copies it on the render worker), so it can be
// made inside the frame. Adding it to ImGui's atlas instead would have the
// WebGPU backend rebuild and upload that from the main thread while the
// render worker submits: Dawn's device is not thread-safe here, and the bigger
// upload crashed early frames.
void load_font(SDL_Window* w) {
    float scale = std::max(1.0f, SDL_GetWindowDisplayScale(w));
    if (const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(w)))
        scale = std::max(scale, mode->h / 900.0f);
    scale = std::min(scale, 4.0f);
    static const ImWchar ranges[] = {0x0020, 0x00FF, 0x2010, 0x205E, 0x2190, 0x2193, 0};
    auto* atlas = new ImFontAtlas();  // for the life of the process
    atlas->Flags |= ImFontAtlasFlags_NoMouseCursors;
    ImFont* font = nullptr;
    char fonts[MAX_PATH];
    const UINT length = GetWindowsDirectoryA(fonts, MAX_PATH);
    if (length > 0 && length < MAX_PATH - 32) {
        const std::string path = std::string(fonts) + "\\Fonts\\segoeui.ttf";
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
            font = atlas->AddFontFromFileTTF(path.c_str(), std::round(15.0f * scale), nullptr, ranges);
    }
    if (font == nullptr) {
        // ImGui's own, at a whole multiple so its pixels stay square.
        scale = std::max(1.0f, std::round(scale));
        ImFontConfig pixel;
        pixel.SizePixels = 13.0f * scale;
        font = atlas->AddFontDefault(&pixel);
    }
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    atlas->GetTexDataAsRGBA32(&pixels, &width, &height);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        std::fprintf(stderr, "[windows] the UI font did not build; keeping ImGui's\n");
        return;
    }
    atlas->SetTexID(aurora_imgui_add_texture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), pixels));
    atlas->ClearTexData();
    ImGui::GetIO().FontDefault = font;
    ImGui::GetStyle().ScaleAllSizes(scale);
    g_font_scale = scale;
    std::fprintf(stderr, "[windows] UI scale %.2f (font atlas %dx%d)\n", scale, width, height);
}

void draw_menu(SDL_Window* w) {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    const float scale = ui_scale(w);
    bluewake_ui::begin_theme(scale);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(io.DisplaySize.x * 0.95f, io.DisplaySize.y * 0.9f));
    ImGui::SetNextWindowBgAlpha(0.94f);
    bool open = true;
    // A scrollbar only when the menu is taller than the window allows (the Mods
    // tab in a small window): at fractional scales the auto-sized window came
    // out a pixel short of its contents and showed one for nothing.
    static bool overflows;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                                   (overflows ? 0 : ImGuiWindowFlags_NoScrollbar);
    if (ImGui::Begin("BlueWake settings", &open, flags)) {
        ImGui::SetWindowFontScale(scale / g_font_scale);
        bluewake_ui::heading("Play your way", "Display, controls and enhancements, all in one place.");
        // Esc normally closes it in the keyboard hook (bw_settings_key), which
        // keeps the key from SDL; one that reaches ImGui instead closes it too.
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            open = false;
        if (ImGui::BeginTabBar("tabs")) {
            if (ImGui::BeginTabItem("Display")) {
                tab_display(w);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Controls")) {
                tab_controls();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Mods")) {
                tab_enhancements();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Sound and files")) {
                tab_game();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::Separator();
        if (needs_restart()) {
            ImGui::TextUnformatted("* Takes effect when BlueWake starts again.");
            ImGui::SameLine();
            if (ImGui::Button("Restart now"))
                restart();
            ImGui::SameLine();
        }
        if (ImGui::Button("Close   (F1 or Esc)"))
            open = false;
        ImGui::SameLine();
        if (ImGui::Button("Quit the game")) {
            open = false;
            if (g_dirty)
                save_file();
            SDL_Event quit{};
            quit.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quit);
        }
        overflows = ImGui::GetScrollMaxY() > 4.0f * scale;
    }
    ImGui::End();
    bluewake_ui::end_theme();
    if (!open)
        set_menu_open(false);
}

// The climbing stamina wheel (climb.c), beside Link in the game's picture: the
// picture is the window's middle at the game's shape, or the whole window when
// "keep the picture's shape" is off (DOL_AURORA_ASPECT_FIT=0, set at launch).
void draw_climb_wheel() {
    float fraction, x, y, aspect, alpha;
    bool exhausted;
    if (!bluewake_climb_hud(&fraction, &exhausted, &x, &y, &aspect, &alpha))
        return;
    static const bool fit = [] {
        const char* v = std::getenv("DOL_AURORA_ASPECT_FIT");
        return v == nullptr || v[0] != '0';
    }();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    float w = display.x, h = display.y, x0 = 0.f, y0 = 0.f;
    if (h <= 0.f || aspect <= 0.f)
        return;
    if (fit && w / h > aspect) {
        w = h * aspect;
        x0 = (display.x - w) * 0.5f;
    } else if (fit) {
        h = w / aspect;
        y0 = (display.y - h) * 0.5f;
    }
    const float radius = h * 0.03f, thick = radius * 0.45f, pi = 3.14159265f;
    const ImVec2 center(x0 + x * w + radius * 2.4f, y0 + y * h - radius * 0.6f);
    ImDrawList* list = ImGui::GetForegroundDrawList();
    const auto a = [alpha](float v) { return static_cast<int>(v * alpha); };
    list->PathArcTo(center, radius, 0.f, 2.f * pi, 48);
    list->PathStroke(IM_COL32(20, 30, 20, a(150.f)), 0, thick + 3.f);
    if (fraction <= 0.002f)
        return;
    ImU32 color = IM_COL32(120, 230, 90, a(245.f));  // green
    if (exhausted) {
        const float pulse = 0.65f + 0.35f * std::sin(static_cast<float>(ImGui::GetTime()) * 8.f);
        color = IM_COL32(235, 70, 50, a(245.f * pulse));  // refilling after running out
    } else if (fraction < 0.25f) {
        color = IM_COL32(245, 190, 60, a(245.f));  // nearly out
    }
    list->PathArcTo(center, radius, -0.5f * pi, -0.5f * pi + 2.f * pi * fraction, 48);
    list->PathStroke(color, 0, thick);
}

// Compile shaders before playing: the game is held at its first present
// (dol_aurora_set_hold, the picture redrawn under this overlay) until every
// pipeline the cache queued at start is compiled, or two minutes have passed.
// They compile on several threads (Aurora's pipeline cache), and while the
// game is held it asks for nothing new. Otherwise they compile while the game
// runs, and a draw whose pipeline is not ready yet is left out of its frame.
bool g_shader_wait;
Uint64 g_shader_wait_since;
uint32_t g_shader_wait_first;

bool shader_wait_hold(void*) {
    if (!g_shader_wait)
        return false;
    const AuroraStats* stats = aurora_get_stats();
    const uint32_t left = stats != nullptr ? stats->queuedPipelines : 0u;
    if (g_shader_wait_since == 0) {
        g_shader_wait_since = SDL_GetTicks();
        g_shader_wait_first = left;
    }
    const Uint64 waited = SDL_GetTicks() - g_shader_wait_since;
    if (left == 0u || waited > 120000) {
        g_shader_wait = false;
        std::fprintf(stderr, "[windows] shaders compiled before play: %u in %.1f s%s\n", g_shader_wait_first,
                     waited / 1000.0, left != 0u ? " (stopped waiting)" : "");
        return false;
    }
    return true;
}

void draw_shader_wait(SDL_Window* w) {
    if (!g_shader_wait)
        return;
    const AuroraStats* stats = aurora_get_stats();
    const uint32_t left = stats != nullptr ? stats->queuedPipelines : 0u;
    ImGuiIO& io = ImGui::GetIO();
    const float scale = ui_scale(w);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.75f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##bluewake-shaders", nullptr, flags)) {
        ImGui::SetWindowFontScale(scale / g_font_scale);
        ImGui::Text("Compiling shaders: %u to go", left);
        if (g_shader_wait_first > 0u)
            ImGui::ProgressBar(1.f - static_cast<float>(left) / static_cast<float>(g_shader_wait_first),
                               ImVec2(260.f * scale, 0.f), "");
    }
    ImGui::End();
}

// For the first few seconds, where the settings and fullscreen are.
void draw_hint(SDL_Window* w) {
    const Uint64 shown = SDL_GetTicks() - g_first_frame_at;
    if (shown > 7000 || g_menu_open)
        return;
    ImGuiIO& io = ImGui::GetIO();
    const float scale = ui_scale(w);
    ImGui::SetNextWindowPos(ImVec2(12 * scale, io.DisplaySize.y - 12 * scale), ImGuiCond_Always, ImVec2(0, 1));
    ImGui::SetNextWindowBgAlpha(shown > 6000 ? 0.6f * (7000 - shown) / 1000.0f : 0.6f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##bluewake-hint", nullptr, flags)) {
        ImGui::SetWindowFontScale(scale / g_font_scale);
        ImGui::TextUnformatted("F1 settings   F11 fullscreen   F9 frame rate   Click the game to steer the camera");
    }
    ImGui::End();
}

// Every presented frame, on the main thread, inside Aurora's frame.
void frame(void*) {
    SDL_Window* w = game_window();
    if (w == nullptr)
        return;
    if (g_first_frame_at == 0) {
        g_first_frame_at = SDL_GetTicks();
        apply_live();
        load_font(w);
        return;  // the font is ImGui's default from the next frame
    }
    if (!g_placed) {
        g_placed = true;
        place_window(w);
    }
    if (g_toggle_fullscreen) {
        g_toggle_fullscreen = false;
        set_fullscreen(w, !is_fullscreen(w));
    }
    if (g_toggle_menu) {
        g_toggle_menu = false;
        set_menu_open(!g_menu_open);
    }
    track_window(w);
    // The frame rate in the session log, a line a second beside the host's
    // [perf] line: frames shown (60 with Smooth Motion at full speed) and the
    // game's own (30 at full speed).
    // With it, where the game thread waited in that second (GXRuntime's
    // counters): for the GX worker at the game's draw-done barriers, and in
    // the present, of which end_frame is Aurora's submission.
    static Uint64 fps_logged;
    static DolAuroraFrameTiming timing_before;
    const Uint64 now = SDL_GetTicks();
    if (now - fps_logged >= 1000) {
        apply_smooth_rate(w);
        DolAuroraFrameTiming timing{};
        dol_aurora_frame_timing(&timing);
        if (fps_logged != 0)
            std::fprintf(stderr, "[fps] shown=%.1f game=%.1f drain_ms=%.0f present_ms=%.0f end_frame_ms=%.0f\n",
                         aurora::gfx::calculate_fps(), aurora::gfx::calculate_game_fps(),
                         (timing.drain_us - timing_before.drain_us) / 1000.0,
                         (timing.present_us - timing_before.present_us) / 1000.0,
                         (timing.end_frame_us - timing_before.end_frame_us) / 1000.0);
        fps_logged = now;
        timing_before = timing;
    }
    // A controller that connects starts from Aurora's mapping: look twice a
    // second whether the one on port 0 changed.
    static unsigned frames;
    apply_key_map();
    if ((++frames % 30u) == 0u) {
        const SDL_JoystickID connection = bw_controller_connection(0);
        if (connection != g_pad_connection) {
            g_pad_connection = connection;
            apply_controller();
        }
    }
    static unsigned healthy_frames;
    if (!g_menu_open && ++healthy_frames == 600u && !bw_launch_clear((g_data_dir + "launch.pending").c_str()))
        std::fprintf(stderr, "[safe-mode] could not clear launch marker\n");
    if (g_safe_mode && SDL_GetTicks() - g_first_frame_at < 15000) {
        ImGui::SetNextWindowBgAlpha(0.85f);
        ImGui::Begin("Launch recovery", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        // A fixed wrap width: TextWrapped in an auto-resizing window wraps to
        // the window's own width and collapses it to a narrow column.
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
        ImGui::TextUnformatted("BlueWake didn't finish starting last time, so this time it starts with Fast "
                               "sound, 4:3 and the mods off. Your earlier settings were saved beside "
                               "settings.ini. You can turn things back on in the settings (F1).");
        ImGui::PopTextWrapPos();
        ImGui::End();
    }
    draw_climb_wheel();
    draw_shader_wait(w);
    if (g_menu_open)
        draw_menu(w);
    draw_hint(w);
    if (g_dirty && SDL_GetTicks() - g_dirty_at > 1000 && !g_menu_open)
        save_file();
}

void save_at_exit() {
    if (g_dirty)
        save_file();
}

}  // namespace

extern "C" void bw_settings_capture_environment(void) {
    wchar_t* block = GetEnvironmentStringsW();
    if (block == nullptr)
        return;
    wchar_t* end = block;
    while (*end != L'\0')
        end += wcslen(end) + 1;
    g_environment.assign(block, end + 1);
    FreeEnvironmentStringsW(block);
}

extern "C" void bw_settings_load(const char* data_dir) {
    g_data_dir = data_dir;
    g_path = g_data_dir + "settings.ini";
    load_file();
    g_safe_mode = env_set("BLUEWAKE_SAFE_MODE") || bw_launch_pending((g_data_dir + "launch.pending").c_str());
    if (g_safe_mode) {
        if (!bw_launch_backup(g_path.c_str())) {
            std::fprintf(stderr, "[safe-mode] settings backup failed; refusing to reset preferences\n");
            // Keep preferences intact; session safety still forces HLE/mods off.
        } else {
            Settings defaults;
            g_saved.aspect = defaults.aspect;
            g_saved.keep_aspect = defaults.keep_aspect;
            g_saved.betterww = false;
            g_saved.options.clear();
            g_saved.hd_textures = false;
            g_saved.lle_audio = false;
            g_saved.movement_extras = false;
            g_saved.fast_transitions = false;
            g_saved.quick_doors = false;
            g_dirty = true;
            save_file();
        }
        _putenv_s("BLUEWAKE_DSP_MODE", "hle");
        _putenv_s("BLUEWAKE_MODS", "none");
        _putenv_s("BLUEWAKE_OPTIONS", "none");
        _putenv_s("BLUEWAKE_ASPECT", "4:3");
        _putenv_s("DOL_AURORA_ASPECT_FIT", "1");
        _putenv_s("DOL_AURORA_TEXTURE_PACK", "");
        _putenv_s("BLUEWAKE_JUMP_BUTTON", "0");
        _putenv_s("BLUEWAKE_SPRINT_SPEED", "1");
        _putenv_s("BLUEWAKE_FAST_FORWARD", "0");
        _putenv_s("BLUEWAKE_FADE_FRAMES", "0");
        _putenv_s("BLUEWAKE_QUICK_DOORS", "0");
        std::fprintf(stderr, "[safe-mode] recovering a pending launch; HLE audio and restart defaults\n");
    }
    g_launched = g_saved;
}

extern "C" void bw_settings_apply_launch(void) {
    // What the command line chose wins for this session (and is shown).
    g_session = g_saved;
    Settings& d = g_session;
    if (env_set("BLUEWAKE_JUMP_BUTTON"))
        d.movement_extras = std::getenv("BLUEWAKE_JUMP_BUTTON")[0] != '0';
    if (env_set("BLUEWAKE_FAST_FORWARD"))
        d.fast_transitions = std::getenv("BLUEWAKE_FAST_FORWARD")[0] != '0';
    if (env_set("BLUEWAKE_QUICK_DOORS"))
        d.quick_doors = std::getenv("BLUEWAKE_QUICK_DOORS")[0] != '0';
    env_default("BLUEWAKE_JUMP_BUTTON", d.movement_extras ? "1" : "0");
    env_default("BLUEWAKE_SPRINT_SPEED", d.movement_extras ? "1.5" : "1");
    env_default("BLUEWAKE_FAST_FORWARD", d.fast_transitions ? "1" : "0");
    env_default("BLUEWAKE_FADE_FRAMES", d.fast_transitions ? "6" : "0");
    env_default("BLUEWAKE_QUICK_DOORS", d.quick_doors ? "1" : "0");
    if (env_set("DOL_AURORA_RENDER_SCALE"))
        d.render_scale = std::clamp(std::atoi(std::getenv("DOL_AURORA_RENDER_SCALE")), 0, 4);
    if (env_set("DOL_AURORA_FORCE_ANISO"))
        d.anisotropy = std::clamp(std::atoi(std::getenv("DOL_AURORA_FORCE_ANISO")), 1, 16);
    if (env_set("DOL_AURORA_FULLSCREEN"))
        d.fullscreen = std::getenv("DOL_AURORA_FULLSCREEN")[0] != '0';
    if (env_set("BLUEWAKE_MOUSE_CAMERA"))
        d.mouse_camera = std::getenv("BLUEWAKE_MOUSE_CAMERA")[0] != '0';
    if (env_set("BLUEWAKE_MOUSE_SENSITIVITY"))
        d.mouse_sensitivity = std::clamp(std::atof(std::getenv("BLUEWAKE_MOUSE_SENSITIVITY")), 0.1, 10.0);
    if (env_set("BLUEWAKE_MOUSE_INVERT_Y"))
        d.mouse_invert_y = std::getenv("BLUEWAKE_MOUSE_INVERT_Y")[0] == '1';
    if (env_set("BLUEWAKE_HAPTICS")) {
        const char mode = std::getenv("BLUEWAKE_HAPTICS")[0];
        d.haptics = mode == 'e' || mode == '1' ? 2 : mode == 'c' ? 1 : 0;
    }
    if (env_set("BLUEWAKE_HAPTICS_STRENGTH"))
        d.haptics_strength = std::clamp(std::atoi(std::getenv("BLUEWAKE_HAPTICS_STRENGTH")), 0, 100);
    if (env_set("BLUEWAKE_HAPTICS_TRIGGERS"))
        d.haptics_triggers = std::getenv("BLUEWAKE_HAPTICS_TRIGGERS")[0] != '0';
    env_default("BLUEWAKE_HAPTICS", haptics_name(d.haptics));
    env_default("BLUEWAKE_HAPTICS_STRENGTH", std::to_string(d.haptics_strength));
    env_default("BLUEWAKE_HAPTICS_TRIGGERS", d.haptics_triggers ? "1" : "0");
    if (d.aspect != "4:3")
        env_default("BLUEWAKE_ASPECT", d.aspect);
    env_default("DOL_AURORA_ASPECT_FIT", d.keep_aspect ? "1" : "0");
    if (d.betterww)
        env_default("BLUEWAKE_MODS", "betterww");
    if (!d.options.empty()) {
        std::string list;
        for (const auto& [name, on] : d.options)
            list += (list.empty() ? "" : ",") + std::string(on ? "" : "-") + name;
        env_default("BLUEWAKE_OPTIONS", list);
    }
    CreateDirectoryA((g_data_dir + "Load").c_str(), nullptr);
    CreateDirectoryA((g_data_dir + "Load\\Textures").c_str(), nullptr);
    CreateDirectoryA(texture_folder().c_str(), nullptr);
    if (d.hd_textures)
        env_default("DOL_AURORA_TEXTURE_PACK", texture_pack_folder(d));
    if (d.lle_audio)
        env_default("BLUEWAKE_DSP_MODE", "lle");
    if (d.fullscreen)
        env_default("DOL_AURORA_FULLSCREEN", "1");
    int width = d.window_w, height = d.window_h;
    if (width < 320 || height < 240) {
        const std::string aspect = env_set("BLUEWAKE_ASPECT") ? std::getenv("BLUEWAKE_ASPECT") : d.aspect;
        default_window(aspect_ratio(aspect), &width, &height);
    }
    env_default("DOL_AURORA_WINDOW", std::to_string(width) + "x" + std::to_string(height));
    env_default("DOL_AURORA_RENDER_SCALE", std::to_string(d.render_scale));
    if (d.anisotropy > 1)
        env_default("DOL_AURORA_FORCE_ANISO", std::to_string(d.anisotropy));
    if (!d.mouse_camera)
        env_default("BLUEWAKE_MOUSE_CAMERA", "0");
    char sensitivity[32];
    std::snprintf(sensitivity, sizeof sensitivity, "%.2f", d.mouse_sensitivity);
    env_default("BLUEWAKE_MOUSE_SENSITIVITY", sensitivity);
    if (d.mouse_invert_y)
        env_default("BLUEWAKE_MOUSE_INVERT_Y", "1");
    // Aurora reads these two before main runs; the command line's --smooth
    // and --fps (which set them) win over the file.
    if (!env_set("DOL_AURORA_FRAME_INTERP"))
        aurora_set_frame_interpolation(d.smooth_motion);
    else
        d.smooth_motion = std::getenv("DOL_AURORA_FRAME_INTERP")[0] == '1';
    if (env_set("DOL_AURORA_FRAME_INTERP_STEPS"))
        d.smooth_steps = bw_smooth_requested(std::getenv("DOL_AURORA_FRAME_INTERP_STEPS"));
    apply_smooth_rate(game_window());
    if (!env_set("DOL_AURORA_SHOW_FPS"))
        aurora_set_fps_overlay(d.show_fps);
    else
        d.show_fps = std::getenv("DOL_AURORA_SHOW_FPS")[0] == '1';
    if (env_set("BLUEWAKE_DSP_MODE")) d.lle_audio = std::strcmp(std::getenv("BLUEWAKE_DSP_MODE"), "lle") == 0;
    if (env_set("BLUEWAKE_ASPECT")) d.aspect = std::getenv("BLUEWAKE_ASPECT");
    if (env_set("DOL_AURORA_ASPECT_FIT")) d.keep_aspect = std::getenv("DOL_AURORA_ASPECT_FIT")[0] != '0';
    if (env_set("BLUEWAKE_MODS")) d.betterww = std::string(std::getenv("BLUEWAKE_MODS")).find("betterww") != std::string::npos;
    if (env_set("DOL_AURORA_TEXTURE_PACK")) d.hd_textures = true;
    if (env_set("BLUEWAKE_OPTIONS")) {
        d.options.clear();
        std::string options = std::getenv("BLUEWAKE_OPTIONS");
        d.option_defaults_off = options.rfind("none", 0) == 0;
        size_t start = 0;
        while (start < options.size()) {
            size_t end = options.find(',', start);
            std::string key = options.substr(start, end - start);
            bool enabled = !key.empty() && key[0] != '-';
            if (!enabled) key.erase(0, 1);
            if (!key.empty() && key != "none") d.options[key] = enabled;
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
    g_launched = g_before_edit = g_session;
}

extern "C" void bw_settings_install(void) {
    dol_aurora_set_overlay(frame, nullptr);
    // BLUEWAKE_SHADERS_FIRST=0/1 overrides the setting (testing).
    const char* first = std::getenv("BLUEWAKE_SHADERS_FIRST");
    g_shader_wait = first != nullptr && first[0] != '\0' ? first[0] != '0' : g_saved.shaders_first;
    if (g_shader_wait) {
        dol_aurora_set_hold(shader_wait_hold, nullptr);
        dol_aurora_set_hold_redraw(true);
    }
    std::atexit(save_at_exit);
}

extern "C" int bw_settings_key(unsigned virtual_key, int alt) {
    switch (virtual_key) {
    case VK_F1:
        g_toggle_menu = true;
        return 1;
    case VK_ESCAPE:
        // Esc first gives the mouse back (the mouse camera sees it); then it
        // opens and closes the menu.
        if (!g_menu_open && bluewake_mouse_camera_captured())
            return 0;
        g_toggle_menu = true;
        return 1;
    case VK_F11:
        g_toggle_fullscreen = true;
        return 1;
    case VK_RETURN:
        if (!alt)
            return 0;
        g_toggle_fullscreen = true;
        return 1;
    case VK_F10:
        g_session.smooth_motion = !aurora_get_frame_interpolation();
        aurora_set_frame_interpolation(g_session.smooth_motion);
        changed();
        std::fprintf(stderr, "[windows] Smooth Motion %s\n", g_session.smooth_motion ? "on" : "off");
        return 1;
    case VK_F6:
        bluewake_save_state_hotkey(false);
        return 1;
    case VK_F8:
        bluewake_save_state_hotkey(true);
        return 1;
    case VK_F9:
        g_session.show_fps = !g_session.show_fps;
        aurora_set_fps_overlay(g_session.show_fps);
        changed();
        return 1;
    default:
        return 0;
    }
}

extern "C" int bw_settings_relaunch(void) {
    return !g_restart.take() || relaunch() ? 0 : 1;
}

extern "C" bool bluewake_settings_menu_event(const void* sdl_event) {
    const SDL_Event* event = static_cast<const SDL_Event*>(sdl_event);
    if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
        set_menu_open(!g_menu_open);
        return true;
    }
    return g_menu_open;
}
