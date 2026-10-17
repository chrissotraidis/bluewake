// SPDX-License-Identifier: GPL-3.0-or-later
// The desktop settings menu, one for Mac, Linux and Windows (desktop_menu.h):
// its window and tabs, the rows with a control of their own, the hotkeys'
// toggles, and the overlays beside it (the climbing wheel, the shader wait,
// safe mode's note). The settings and their file are desktop_settings.cpp;
// each tab is settings_table.h's rows for it, drawn by menu_draw.h.
#include "desktop_menu.h"
#include "desktop_settings.h"
// ImGui first: button_remap.h and input_remap.h draw their rows only with it.
#include <imgui.h>
#include <imgui_internal.h> // FindWindowByName: the menu's position before Begin draws it
#include "button_remap.h"
#include "controller_face_swap.h"
#include "desktop_theme.h"
#include "fps_position.h"
#include "input_remap.h"
#include "menu_draw.h"
#include "menu_shared.h"
#include "option_notes.h"
#include "smooth_rate.h"
#include "stage_select_key.h"
#include "status_line_overlay.h"
#include "texture_pack_menu.h"

// The host's modules are C.
extern "C" {
#include "climb.h"
#include "dpad_menus.h"
#include "fast_load.h"
#include "forest_water.h"
#include "haptics.h"
#include "hd_look.h"
#include "jump_button.h"
#include "mouse_camera.h"
#include "quick_doors.h"
#include "save_state.h"
#include "sprint.h"
}

#include "gxruntime/aurora_backend.h"

#include <SDL3/SDL.h>
#include <aurora/aurora.h>
#include <aurora/gfx.h>
#include <aurora/imgui.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <system_error>
#include <vector>

extern "C" {
void aurora_set_frame_buffer_scale(float scale);
void aurora_set_frame_interpolation(bool enabled);
void aurora_set_frame_interp_steps(int steps);
void aurora_set_fps_overlay(bool enabled);
void aurora_set_forced_anisotropy(unsigned samples);
}

namespace bw_desktop {
namespace {

bool g_open = false;
// Whether the game waits while the menu is open (BLUEWAKE_MENU_PAUSES), read
// at the first frame and when the player changes it.
bool g_menu_pauses = false;
bw_texture_pack::Counter g_pack_count; // textures in the pack, counted once per opening
bool g_stage_select_question = false;  // the menu shows only the stage select's question
Uint64 g_safe_mode_since = 0;
Uint64 g_installed_ms = 0;
// BLUEWAKE_SETTINGS_TEST_OPEN=at:for (seconds after the menu is installed;
// testing only): opens the menu, and closes it after `for` seconds.
double g_test_open_at = -1.0, g_test_open_for = 0.0;
bool g_test_done = false;

// What the lists of choices offer (bw_menu::choices): shown, and stored.
// Render resolution by its multiple, not by pixels: the size the game asks
// for is its own (a GameCube mode is not always 480 lines), and the width
// comes from the widescreen mod when there is one and from the window when
// there is not, so any width by height here would be a guess.
const char* const kScaleShown[] = {"Window", "1x", "2x", "3x", "4x"};
const char* const kScaleStored[] = {"0", "1", "2", "3", "4"};
const char* const kAnisoShown[] = {"Off (the game's)", "2x", "4x", "8x", "16x"};
const char* const kAnisoStored[] = {"1", "2", "4", "8", "16"};

// Beside the control when it fits there. A list or a slider fills its line,
// and after one the note went past the edge and, wrapped to nothing there, was
// a column of letters nobody could see: an empty space under the control.
void restart_note() {
    const char* const note = "(next launch)";
    const float room = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x - ImGui::GetItemRectMax().x;
    if (room >= ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(note).x)
        ImGui::SameLine();
    ImGui::TextDisabled("%s", note);
}

// --- settings that apply at once ----------------------------------------------

float display_refresh() {
    SDL_Window* window = game_window();
    const SDL_DisplayMode* mode = window != nullptr ? SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window)) : nullptr;
    return mode != nullptr ? mode->refresh_rate : 0.f;
}

int smooth_requested() { return bw_smooth_requested(env("DOL_AURORA_FRAME_INTERP_STEPS", "1").c_str()); }

void refresh_smooth_rate() { aurora_set_frame_interp_steps(bw_smooth_steps(smooth_requested(), display_refresh())); }

void apply_fps_position() {
    aurora_set_fps_overlay_position(
        static_cast<AuroraFpsOverlayPosition>(bw_fps_position(env("DOL_AURORA_FPS_POSITION").c_str())));
}

// steps: "1", "3" or "display"; nullptr turns it off. The switch and its rate
// are one choice, so F10 turns it on at the rate last chosen.
void set_smooth_motion(const char* steps) {
    set_env("DOL_AURORA_FRAME_INTERP", steps != nullptr ? "1" : "0");
    if (steps != nullptr)
        set_env("DOL_AURORA_FRAME_INTERP_STEPS", steps);
    refresh_smooth_rate();
    aurora_set_frame_interpolation(steps != nullptr);
}

bool is_fullscreen() {
    SDL_Window* window = game_window();
    return window != nullptr && (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
}

// For this session only: whether the game starts in fullscreen is its own
// setting (DOL_AURORA_FULLSCREEN, read at launch), so trying fullscreen never
// changes how the game starts (#320).
void set_fullscreen(bool on) {
    if (SDL_Window* window = game_window()) {
        SDL_SetWindowFullscreen(window, on);
        std::fprintf(stderr, "[settings] fullscreen %s\n", on ? "on" : "off");
    }
}

// The controller's buttons as the player set them: the swaps, or a custom
// layout instead of them, the triggers, and the camera stick inverted. Applied
// whenever they or the controller on port 0 change; a mapping set elsewhere is
// left alone until the player touches one of these.
void apply_controller() {
    static SDL_JoystickID previous = 0;
    static bool applied = false;
    static std::string last;
    const SDL_JoystickID connection = bw_controller_connection(0);
    const std::string map_text = env("BLUEWAKE_BUTTON_MAP");
    const bool ab = env_on("BLUEWAKE_PAD_SWAP_AB", false), xy = env_on("BLUEWAKE_PAD_SWAP_XY", false);
    const bool lr = env_on("BLUEWAKE_PAD_SWAP_LR", false);
    // The camera stick's inverts at the pad, for where the game's own camera
    // has the view. Still as each menu had it on main, until it is decided:
    // Windows always; Mac and Linux only with the direct stick camera on, so
    // with it off Better Wind Waker's "Invert camera" is the only invert (its
    // note says so) and the two cannot cancel out.
#if defined(_WIN32)
    const bool pad_inverts = true;
#else
    const bool pad_inverts = env_on("BLUEWAKE_STICK_CAMERA", true);
#endif
    const bool ix = pad_inverts && env_on("BLUEWAKE_STICK_CAMERA_INVERT_X", false);
    const bool iy = pad_inverts && env_on("BLUEWAKE_STICK_CAMERA_INVERT_Y", false);
    const std::string state = map_text + (ab ? "a" : "") + (xy ? "x" : "") + (lr ? "l" : "") + (ix ? "h" : "") +
                              (iy ? "v" : "");
    if (connection == previous && state == last)
        return;
    BwButtonMap map;
    const bool remapped = bw_button_map_parse(map_text, &map);
    if (PADGetIndexForPort(0) < 0 || (!ab && !xy && !lr && !ix && !iy && !remapped && !applied)) {
        previous = connection;
        last = state;
        return;
    }
    previous = connection;
    last = state;
    PADRestoreDefaultMapping(0);
    if (remapped)
        bw_apply_button_map(0, map);
    else
        bw_apply_face_swaps(0, ab, xy);
    bw_apply_camera_axes(0, ix, iy);
    bw_apply_trigger_swap(0, lr);
    applied = true;
}

// The keyboard's keys for the GameCube buttons (input_remap.h), applied once
// the pad's keyboard bindings exist and again whenever they change.
void apply_key_map() {
    static bool applied = false;
    static std::string last;
    const std::string text = env("BLUEWAKE_KEY_MAP");
    if (applied && text == last)
        return;
    BwKeyMap map;
    bw_key_map_parse(text, &map);
    if (bw_apply_key_map(0, map)) {
        applied = true;
        last = text;
    }
}

// Every setting the game and Aurora read while they run, from the variables:
// the first frame, once the launch has filled them in.
void read_menu_pauses() {
    g_menu_pauses =
        env_on("BLUEWAKE_MENU_PAUSES", bw_settings::default_on(bw_menu::setting("BLUEWAKE_MENU_PAUSES")));
}

void apply_live() {
    read_menu_pauses();
    aurora_set_frame_buffer_scale(static_cast<float>(env_int("DOL_AURORA_RENDER_SCALE", 0)));
    aurora_set_forced_anisotropy(static_cast<unsigned>(env_int("DOL_AURORA_FORCE_ANISO", 1)));
    refresh_smooth_rate();
    aurora_set_frame_interpolation(env_on("DOL_AURORA_FRAME_INTERP", false));
    aurora_set_fps_overlay(env_on("DOL_AURORA_SHOW_FPS", false));
    apply_fps_position();
    aurora_set_pause_on_focus_lost(env_on("BLUEWAKE_PAUSE_UNFOCUSED", false));
    bluewake_mouse_camera_reload();  // the mouse, both sticks, gyro and the mouse's buttons
    bluewake_haptics_reload();
    bluewake_climb_reload();
    bluewake_forest_water_reload();
    bluewake_hd_look_reload();
    bluewake_dpad_menus_reload();
}

// Applied the moment a setting changes, from its variable. A setting this
// session takes at the next launch is only written down.
void apply_now(const bw_settings::Setting& s) {
    const char* key = s.key;
    const std::string value = env(key);
    // The frames a fade skips follow Fast transitions; both keep working on
    // their own for testing.
    if (std::strcmp(key, "BLUEWAKE_FAST_FORWARD") == 0)
        set_env("BLUEWAKE_FADE_FRAMES", value != "0" ? "6" : "0");
    if (next_launch(s))
        return;
    if (std::strncmp(key, "BLUEWAKE_HD_", 12) == 0)  // the HD look and its parts (sun shadows)
        bluewake_hd_look_reload();
    else if (std::strcmp(key, "BLUEWAKE_MENU_PAUSES") == 0)
        read_menu_pauses();
    else if (std::strcmp(key, "DOL_AURORA_SHOW_FPS") == 0)
        aurora_set_fps_overlay(value != "0");
    else if (std::strcmp(key, "DOL_AURORA_FPS_POSITION") == 0)
        apply_fps_position();
    else if (std::strcmp(key, "DOL_AURORA_RENDER_SCALE") == 0)
        aurora_set_frame_buffer_scale(static_cast<float>(std::atoi(value.c_str())));
    else if (std::strcmp(key, "DOL_AURORA_FORCE_ANISO") == 0)
        aurora_set_forced_anisotropy(static_cast<unsigned>(std::atoi(value.c_str())));
    else if (std::strcmp(key, "BLUEWAKE_PAUSE_UNFOCUSED") == 0)
        aurora_set_pause_on_focus_lost(value != "0");
    else if (std::strcmp(key, "BLUEWAKE_FOREST_WATER_KEEP_TREES") == 0 ||
             std::strcmp(key, "BLUEWAKE_FOREST_WATER_30_MINUTES") == 0)
        bluewake_forest_water_reload();
    else if (std::strcmp(key, "BLUEWAKE_QUICK_DOORS") == 0)
        bluewake_quick_doors_reload();
    else if (std::strcmp(key, "BLUEWAKE_FAST_FORWARD") == 0)
        bluewake_fast_load_reload();
    else if (std::strcmp(key, "BLUEWAKE_CLIMB") == 0 || std::strcmp(key, "BLUEWAKE_CLIMB_STAMINA") == 0)
        bluewake_climb_reload();
    else if (std::strcmp(key, "BLUEWAKE_DPAD_MENUS") == 0)
        bluewake_dpad_menus_reload();
    // Everything the mouse, the sticks and gyro aiming do is read back together.
    else if (std::strncmp(key, "BLUEWAKE_MOUSE_", 15) == 0 || std::strncmp(key, "BLUEWAKE_STICK_", 15) == 0 ||
             std::strncmp(key, "BLUEWAKE_GYRO_", 14) == 0 || std::strcmp(key, "BLUEWAKE_AIM_INVERT_Y") == 0)
        bluewake_mouse_camera_reload();
    else if (std::strncmp(key, "BLUEWAKE_HAPTICS", 16) == 0)
        bluewake_haptics_reload();
    // The controller's swaps and layout, and the keyboard's keys, are applied
    // every frame when they change (apply_controller, apply_key_map).
}

// --- the custom rows ---------------------------------------------------------

// Sliders as the shared ones (menu_shared.h), for the rows that draw their own.
bool slider(const char* label, float* value, float low, float high, const char* format) {
    return bw_menu_slider(label, value, low, high, format);
}

// Wind Waker HD textures: the same section, in the same order, as the iPhone
// and iPad Mods menu (texture_pack_menu.h). They are on while a folder is
// chosen, as in the settings file.
void hd_textures_section() {
    std::string picked;
    if (bw_texture_pack::take_folder(picked))
        set_texture_pack(picked);
    ImGui::SeparatorText("Wind Waker HD textures");
    bool on = !launch_choices().texture_pack.empty();
    if (bw_menu_checkbox("HD textures", &on)) {
        if (!on) {
            set_texture_pack("");
        } else {
            const std::string folder = !launch_choices().last_pack.empty() ? launch_choices().last_pack : bw_platform::default_pack_folder();
            if (folder.empty())
                bw_texture_pack::choose_folder(game_window(), "");
            else
                set_texture_pack(folder);
        }
    }
    restart_note();
    on = !launch_choices().texture_pack.empty();
    const size_t textures = g_pack_count.get(launch_choices().texture_pack);
    {
        const bw_texture_pack::PackState state = bw_texture_pack::state_of(textures, on);
        std::string line;
        if (!on)
            line = "Off. A Dolphin-format pack for GZLE01 (a folder of .png or .dds files) replaces the game's textures.";
        else if (textures == 0)
            line = "No .png or .dds textures found in the chosen folder.";
        else
            line = std::to_string(textures) + " textures in " + launch_choices().texture_pack;
        ImGui::TextColored(bw_texture_pack::state_color(state), "%s", line.c_str());
    }
    const std::string default_folder = bw_platform::default_pack_folder();
    if (ImGui::SmallButton("Choose texture pack folder..."))
        bw_texture_pack::choose_folder(game_window(), on ? launch_choices().texture_pack : default_folder);
    const std::string shown = on ? launch_choices().texture_pack : default_folder;
    if (!shown.empty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Open the texture folder"))
            bw_platform::open_folder(shown);
    }
    if (on && !default_folder.empty() && launch_choices().texture_pack != default_folder) {
        ImGui::SameLine();
        const std::string use = "Use " + default_folder + " instead";
        if (ImGui::SmallButton(use.c_str()))
            set_texture_pack(default_folder);
    }
    std::string imported;
    if (bw_texture_pack::import_controls(game_window(), bw_platform::textures_folder(), imported))
        set_texture_pack(imported);
    const std::string error = bw_texture_pack::picker_error();
    if (!error.empty()) {
        ImGui::TextWrapped("The folder picker did not open (%s). Type the folder instead:", error.c_str());
        static char typed[1024];
        static bool typing = false;
        if (!typing)
            std::snprintf(typed, sizeof typed, "%s", launch_choices().texture_pack.c_str());
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::InputText("##texpack", typed, sizeof typed))
            set_texture_pack(typed);
        typing = ImGui::IsItemActive();
    }
    ImGui::Spacing();
}

// A Custom row draws itself: the switch that needs a button beside it, the
// settings that are one choice, the tables of keys and buttons, and the
// next-launch choices this menu keeps a copy of.
bool draw_custom(const bw_settings::Setting& s) {
    if (std::strcmp(s.key, "DOL_AURORA_FULLSCREEN") == 0) {
        // An action for this session, not a setting: a button above the switch.
        if (ImGui::Button(is_fullscreen() ? "Leave fullscreen   (F11 or Alt+Enter)" : "Fullscreen   (F11 or Alt+Enter)"))
            toggle_fullscreen();
        bool start_fullscreen = false;
        bw_menu::draw_switch(s, &start_fullscreen);
        restart_note();
        return true;
    }
    if (std::strcmp(s.key, "DOL_AURORA_FRAME_INTERP") == 0) {
        // The switch and its rate are one list. The display's choice is named
        // Display, not its rate, which can be one of the fixed choices (120 Hz);
        // the line under it says what the chosen one comes to here, a whole
        // number of frames to each of the game's (up to 180 FPS on 200 Hz).
        const float refresh = display_refresh();
        const char* const kSmooth[] = {"Off (30 FPS)", "60 FPS", "120 FPS", "Display"};
        const int requested = smooth_requested();
        int smooth = !env_on(s.key, false) ? 0 : requested == -1 ? 3 : requested >= 3 ? 2 : 1;
        if (bw_menu_combo(s.label, &smooth, kSmooth, 4))
            set_smooth_motion(smooth == 0 ? nullptr : smooth == 3 ? "display" : smooth == 2 ? "3" : "1");
        const int most = env_on(s.key, false) ? (bw_smooth_steps(smooth_requested(), refresh) + 1) * 30 : 30;
        if (refresh > 0.f)
            ImGui::TextDisabled("    Up to %d FPS on this %.0f Hz display; pacing may reduce it.", most, refresh);
        else
            ImGui::TextDisabled("    Up to %d FPS; the display's rate isn't known yet.", most);
        return true;
    }
    if (std::strcmp(s.key, "DOL_AURORA_FPS_POSITION") == 0) {
        // On a picture of the screen, in the menu's order (fps_position.h).
        const char* names[BW_FPS_POSITIONS];
        ImVec2 spots[BW_FPS_POSITIONS];
        const int position = bw_fps_position(env(s.key).c_str());
        int at = 0;
        for (int i = 0; i < BW_FPS_POSITIONS; ++i) {
            names[i] = kBwFpsPositionNames[kBwFpsPositionOrder[i]];
            spots[i] = ImVec2(kBwFpsPositionSpot[i][0], kBwFpsPositionSpot[i][1]);
            if (kBwFpsPositionOrder[i] == position)
                at = i;
        }
        if (bw_menu_spot_chooser(s.label, &at, names, spots, BW_FPS_POSITIONS)) {
            set_env(s.key, kBwFpsPositionValues[kBwFpsPositionOrder[at]]);
            apply_fps_position();
        }
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_BUTTON_MAP") == 0) {
        if (!ImGui::CollapsingHeader(s.label))
            return true;
        ImGui::TextWrapped("Choose which controller button presses each GameCube button. Picking one that is "
                           "already used swaps the two.");
        BwButtonMap map;
        bw_button_map_parse(env(s.key), &map);
        if (bw_button_map_ui(&map))
            set_env(s.key, bw_button_map_format(map));
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_KEY_MAP") == 0) {
        if (!ImGui::CollapsingHeader(s.label))
            return true;
        ImGui::TextWrapped("Choose the key for each GameCube button. Picking one that is already used swaps the two.");
        BwKeyMap keys;
        bw_key_map_parse(env(s.key), &keys);
        if (bw_key_map_ui(&keys))
            set_env(s.key, bw_key_map_format(keys));
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_MOUSE_BUTTONS") == 0) {
        if (!ImGui::CollapsingHeader(s.label))
            return true;
        ImGui::TextWrapped("What each mouse button presses while the mouse is the camera. The first left click "
                           "only hands the mouse to the game.");
        BwMouseMap buttons;
        bw_mouse_map_parse(env(s.key).c_str(), &buttons);
        if (bw_mouse_map_ui(&buttons)) {
            char text[64];
            bw_mouse_map_format(&buttons, text, sizeof text);
            set_env(s.key, text);
            bluewake_mouse_camera_reload();
        }
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_GYRO_AIM_SPEED") == 0) {
        // The slider, and under it what the right stick does, which depends on
        // the direct camera above rather than on this setting.
        if (bw_menu::draw_slider(s))
            bluewake_mouse_camera_reload();
        ImGui::PushTextWrapPos();
        ImGui::TextDisabled(env_on("BLUEWAKE_STICK_CAMERA", true)
                                ? "Click the right stick for first person. When controller zoom is on, hold it and "
                                  "move the stick up/down. In the telescope and Picto Box, the left stick or D-pad zooms."
                                : "The game's right stick: its left and right follow Better Wind Waker's "
                                  "\"Invert camera\" (Gameplay).");
        ImGui::PopTextWrapPos();
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_HAPTICS") == 0) {
        // The three vibration settings move together: the kind, how strong, and
        // the triggers, which only the shaped kind uses.
        static const char* const kHaptics[] = {"Off", "Classic (the game's own on and off)",
                                               "Enhanced (shaped, with the triggers)"};
        const std::string mode = env(s.key, bw_settings::fallback_for(s));
        int haptics = mode.empty() || mode[0] == 'e' || mode[0] == '1' ? 2 : mode[0] == 'c' ? 1 : 0;
        if (bw_menu_combo(s.label, &haptics, kHaptics, 3)) {
            set_env(s.key, haptics == 0 ? "off" : haptics == 1 ? "classic" : "enhanced");
            bluewake_haptics_reload();
        }
        ImGui::BeginDisabled(haptics != 2);
        float strength = static_cast<float>(std::clamp(env_int("BLUEWAKE_HAPTICS_STRENGTH", 80), 0, 100));
        if (slider(bw_menu::setting("BLUEWAKE_HAPTICS_STRENGTH").label, &strength, 0.f, 100.f, "%.0f%%")) {
            set_env("BLUEWAKE_HAPTICS_STRENGTH", std::to_string(static_cast<int>(std::lround(strength))));
            bluewake_haptics_reload();
            bluewake_haptics_preview();  // a buzz at the new strength
        }
        bool triggers = false;
        if (bw_menu::draw_switch(bw_menu::setting("BLUEWAKE_HAPTICS_TRIGGERS"), &triggers))
            bluewake_haptics_reload();
        ImGui::EndDisabled();
        ImGui::TextDisabled(haptics == 2 ? "Hits, falls, explosions and quakes as the game times them, shaped by "
                                           "their strength."
                            : haptics == 1 ? "The motor on and off, as a GameCube controller's."
                                           : "No vibration. (The game's own Vibration option turns it off too.)");
        return true;
    }
    if (std::strcmp(s.key, "DOL_AURORA_TEXTURE_PACK") == 0) {
        hd_textures_section();
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_ASPECT") == 0) {
        // The chosen shape is the menu's copy (launch_choices): the picture, the mods and the
        // Display tab's "keep the shape" all follow it.
        static const char* const kAspects[] = {"Off (4:3, the game's)", "16:10", "16:9", "21:9"};
        static const char* const kAspectValues[] = {"4:3", "16:10", "16:9", "21:9"};
        int aspect = 0;
        for (int i = 0; i < 4; ++i)
            if (launch_choices().aspect == kAspectValues[i])
                aspect = i;
        if (bw_menu_combo(s.label, &aspect, kAspects, 4)) {
            launch_choices().aspect = kAspectValues[aspect];
            choice_changed(s.key);
        }
        restart_note();
        ImGui::TextDisabled("Widescreen widens the camera and moves the HUD to the edges.");
        ImGui::Spacing();
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_MODS") == 0) {
        // Better Wind Waker, and its own options once the game has read them.
        if (bw_menu_checkbox(s.label, &launch_choices().betterww))
            choice_changed(s.key);
        restart_note();
        if (launch_choices().options.empty()) {
            ImGui::TextDisabled("This build has no Better Wind Waker options (build with mods).");
            return true;
        }
        ImGui::TextDisabled("Wind Waker HD's changes. Each can be turned on or off:");
        ImGui::BeginDisabled(!launch_choices().betterww);
        ImGui::Indent();
        for (size_t i = 0; i < launch_choices().options.size(); ++i) {
            bool on = launch_choices().options[i].second;
            ImGui::PushID(launch_choices().options[i].first.c_str());
            if (bw_menu_checkbox(launch_choices().option_titles[i].c_str(), &on)) {
                launch_choices().options[i].second = on;
                choice_changed("BLUEWAKE_OPTIONS");
            }
            if (const char* note = bw_option_note(launch_choices().options[i].first.c_str())) {
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
                ImGui::TextDisabled("%s", note);
                ImGui::PopTextWrapPos();
            }
            ImGui::PopID();
        }
        ImGui::Unindent();
        ImGui::EndDisabled();
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_DSP_MODE") == 0) {
        // Two ways to make the sound: a switch, not a list of two.
        bool exact = env(s.key, bw_settings::fallback_for(s)) == "lle";
        if (bw_menu_checkbox("Exact sound (the sound chip's own program)", &exact))
            set_env(s.key, exact ? "lle" : "hle");
        restart_note();
        ImGui::TextDisabled("Off: Dolphin's high-level Zelda sound, which is faster. On: the chip's own "
                            "program, which is slower but exact.");
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_STAGE_SELECT_TOGGLE") == 0) {
        bool stage_select = false;
        bw_menu::draw_switch(s, &stage_select);
        restart_note();
        if (s.note != nullptr)
            ImGui::TextWrapped("%s", s.note);
        bw_stage_select_save_warning();
        if (bw_stage_select_button())
            close_menu();
        return true;
    }
    if (std::strcmp(s.key, "BLUEWAKE_JUMP_BUTTON") == 0) {
        // Jump & Sprint, one switch as on the iPad, with the sprint's speed
        // under it. The chosen speed is kept apart (BLUEWAKE_SPRINT_CHOICE) so
        // turning it off (BLUEWAKE_SPRINT_SPEED 1) and on again keeps it.
        const float current = static_cast<float>(std::atof(env("BLUEWAKE_SPRINT_SPEED", "1").c_str()));
        bool extras = env_on(s.key, false) || current > 1.01f;
        float choice = static_cast<float>(std::atof(env("BLUEWAKE_SPRINT_CHOICE", "0").c_str()));
        if (choice < 1.1f || choice > 2.f)
            choice = current > 1.01f && current <= 2.f ? std::max(current, 1.1f) : 1.5f;
        const bool later = next_launch(s);
        const auto apply_sprint = [&]() {
            char text[16];
            std::snprintf(text, sizeof text, "%.2f", choice);
            set_env("BLUEWAKE_SPRINT_CHOICE", text);
            set_env("BLUEWAKE_SPRINT_SPEED", extras ? text : "1");
            set_env(s.key, extras ? "1" : "0");
            if (!later) {
                bluewake_jump_button_reload();
                bluewake_sprint_reload();
            }
        };
        if (bw_menu_checkbox(s.label, &extras))
            apply_sprint();
        if (later)
            restart_note();
        ImGui::BeginDisabled(!extras);
        if (slider(bw_menu::setting("BLUEWAKE_SPRINT_CHOICE").label, &choice, 1.1f, 2.f, "%.2fx"))
            apply_sprint();
        ImGui::EndDisabled();
        return true;
    }
    if (std::strcmp(s.key, "DOL_AURORA_ASPECT_FIT") == 0) {
        // When the window isn't the picture's shape: black bars, or the picture
        // stretched to fill it. A widened picture (16:10, 16:9, 21:9) always
        // keeps its shape, so the choice is only for 4:3.
        if (launch_choices().aspect != "4:3") {
            ImGui::TextDisabled("Widened pictures always keep their shape (black bars rather than stretching).");
            return true;
        }
        bool keep_shape = false;
        bw_menu::draw_switch(s, &keep_shape);
        restart_note();
        return true;
    }
    return false;
}

}  // namespace
}  // namespace bw_desktop

// What the shared drawing (menu_draw.h) asks of the menu.
namespace bw_menu {
std::string get(const char* key, const char* fallback) { return bw_desktop::env(key, fallback); }
void put(const char* key, const std::string& value) { bw_desktop::set_env(key, value); }
void next_launch_note(const bw_settings::Setting& s) {
    if (bw_desktop::next_launch(s))
        bw_desktop::restart_note();
}
bool custom(const bw_settings::Setting& s) { return bw_desktop::draw_custom(s); }
bool choices(const char* key, const char* const** shown, const char* const** stored, int* count) {
    if (std::strcmp(key, "DOL_AURORA_RENDER_SCALE") == 0) {
        *shown = bw_desktop::kScaleShown;
        *stored = bw_desktop::kScaleStored;
        *count = 5;
        return true;
    }
    if (std::strcmp(key, "DOL_AURORA_FORCE_ANISO") == 0) {
        *shown = bw_desktop::kAnisoShown;
        *stored = bw_desktop::kAnisoStored;
        *count = 5;
        return true;
    }
    return false;
}
// put has set the variable and marked the file; this hands it to the game.
void changed(const bw_settings::Setting& s) { bw_desktop::apply_now(s); }
}  // namespace bw_menu

namespace bw_desktop {
namespace {

// --- the tabs ----------------------------------------------------------------

// Each tab is settings_table.h's rows for it, in the table's order. What else
// a tab holds is not a setting: an action, or something to read.
void display_tab() {
    bw_menu::draw_tab(bw_settings::Tab::Display);
    ImGui::Spacing();
    if (ImGui::Button("Reset the window"))
        bw_platform::reset_window();
    ImGui::SameLine();
    ImGui::TextDisabled("Out of fullscreen, at its starting size, in the middle of the screen.");
}

void gameplay_tab() { bw_menu::draw_tab(bw_settings::Tab::Gameplay); }

// Mods: HD textures first, then widescreen and Better Wind Waker, as in the
// iPhone and iPad Mods menu.
void mods_tab() {
    bw_menu::draw_tab(bw_settings::Tab::Mods);
    bw_menu::draw_experimental();
}

void controls_tab() {
    bw_menu::draw_tab(bw_settings::Tab::Controls);
    // The keys as they are now (a remapped key shows its new name); then the
    // mouse and the controller.
    ImGui::SeparatorText("Keyboard");
    BwKeyMap keys;
    bw_key_map_parse(env("BLUEWAKE_KEY_MAP"), &keys);
    bw_keyboard_table(keys);
    ImGui::TextWrapped("Mouse: click the game, then move it to turn the camera and aim; left click is A and the "
                       "wheel zooms. Esc gives the mouse back, and Esc again opens this menu.");
    ImGui::SeparatorText("Controller");
    bw_controller_table();
    ImGui::TextDisabled("Game controllers work as they are plugged in (Xbox, PlayStation, Switch Pro, ...).");
}

// Sound and files: the sound's engine (next launch), and where the saves,
// settings and logs are.
void sound_files_tab() {
    bw_menu::draw_tab(bw_settings::Tab::SoundFiles);
    ImGui::Spacing();
    ImGui::SeparatorText("Your files");
    std::string folder = data_dir();
    if (folder.empty() && !settings_path().empty())
        folder = std::filesystem::path(settings_path()).parent_path().string();
    if (folder.empty()) {
        ImGui::TextDisabled("This session keeps no files.");
        return;
    }
    ImGui::TextDisabled("Saves, settings and logs are kept apart from the build:");
    ImGui::TextUnformatted(folder.c_str());
    if (!settings_path().empty())
        ImGui::TextDisabled("Settings: %s", settings_path().c_str());
    if (ImGui::Button("Open that folder"))
        bw_platform::open_folder(folder);
    std::error_code error;
    const std::string logs = (std::filesystem::path(folder) / "logs").string();
    if (std::filesystem::is_directory(logs, error)) {
        ImGui::SameLine();
        if (ImGui::Button("Open the session logs"))
            bw_platform::open_folder(logs);
    }
}

// --- the overlays --------------------------------------------------------------

// Compile shaders before playing (BLUEWAKE_SHADERS_FIRST=1): the game is held
// at its first present (dol_aurora_set_hold, the picture redrawn under this
// overlay) until every pipeline the cache queued at start is compiled, or two
// minutes have passed. Otherwise they compile while the game runs, and a draw
// whose pipeline is not ready yet is left out of its frame.
bool g_shader_wait = false;
Uint64 g_shader_wait_since = 0;
uint32_t g_shader_wait_first = 0;

bool shader_wait_holds() {
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
        std::fprintf(stderr, "[settings] shaders compiled before play: %u in %.1f s%s\n", g_shader_wait_first,
                     waited / 1000.0, left != 0u ? " (stopped waiting)" : "");
        return false;
    }
    return true;
}

// The game waits while the shaders compile, and while the menu is open if
// the player has it pause.
bool hold(void*) { return (g_open && g_menu_pauses) || shader_wait_holds(); }

constexpr ImGuiWindowFlags kNotice = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

void draw_shader_wait(const bw_platform::Look& look) {
    if (!g_shader_wait)
        return;
    const AuroraStats* stats = aurora_get_stats();
    const uint32_t left = stats != nullptr ? stats->queuedPipelines : 0u;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.75f);
    if (look.font != nullptr)
        ImGui::PushFont(look.font);
    if (ImGui::Begin("##bluewake-shaders", nullptr, kNotice)) {
        ImGui::SetWindowFontScale(look.font_scale);
        ImGui::Text("Compiling shaders: %u to go", left);
        if (g_shader_wait_first > 0u)
            ImGui::ProgressBar(1.f - static_cast<float>(left) / static_cast<float>(g_shader_wait_first),
                               ImVec2(260.f * look.scale, 0.f), "");
    }
    ImGui::End();
    if (look.font != nullptr)
        ImGui::PopFont();
}

// Safe mode's note, for the first fifteen seconds.
void draw_safe_mode_note(Uint64 now, const bw_platform::Look& look) {
    if (!safe_mode())
        return;
    if (g_safe_mode_since == 0)
        g_safe_mode_since = now;
    if (now - g_safe_mode_since > 15000)
        return;
    ImGui::SetNextWindowBgAlpha(0.85f);
    if (look.font != nullptr)
        ImGui::PushFont(look.font);
    if (ImGui::Begin("Launch recovery", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
        ImGui::SetWindowFontScale(look.font_scale);
        // A fixed wrap width: in an auto-resizing window, wrapping to the
        // window's own width collapses it to a narrow column.
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.f);
        ImGui::TextUnformatted(safe_backed_up()
                                   ? "BlueWake didn't finish starting last time, so this time it starts with Fast "
                                     "sound, 4:3, in a window and with the mods off. Your earlier settings were saved "
                                     "beside settings.ini. You can turn things back on in the settings (F1)."
                                   : "BlueWake didn't finish starting last time, so this time it starts with Fast "
                                     "sound, 4:3, in a window and with the mods off, for this session only: your "
                                     "settings come back at the next launch.");
        ImGui::PopTextWrapPos();
    }
    ImGui::End();
    if (look.font != nullptr)
        ImGui::PopFont();
}

// The climbing stamina wheel (climb.c), beside Link in the game's picture: the
// picture is the window's middle at the game's shape, or the whole window when
// "keep the picture's shape" is off (DOL_AURORA_ASPECT_FIT=0, read at launch).
void draw_climb_wheel() {
    float fraction, x, y, aspect, alpha;
    bool exhausted;
    if (!bluewake_climb_hud(&fraction, &exhausted, &x, &y, &aspect, &alpha))
        return;
    static const bool fit = env_on("DOL_AURORA_ASPECT_FIT", true) || env("BLUEWAKE_ASPECT", "4:3") != "4:3";
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
    ImU32 color = IM_COL32(120, 230, 90, a(245.f)); // green
    if (exhausted) {
        const float pulse = 0.65f + 0.35f * std::sin(static_cast<float>(ImGui::GetTime()) * 8.f);
        color = IM_COL32(235, 70, 50, a(245.f * pulse)); // refilling after running out
    } else if (fraction < 0.25f) {
        color = IM_COL32(245, 190, 60, a(245.f)); // nearly out
    }
    list->PathArcTo(center, radius, -0.5f * pi, -0.5f * pi + 2.f * pi * fraction, 48);
    list->PathStroke(color, 0, thick);
}

void test_hook() {
    if (g_test_open_at < 0.0 || g_test_done)
        return;
    const double t = (SDL_GetTicks() - g_installed_ms) / 1000.0;
    if (!g_open && t >= g_test_open_at && t < g_test_open_at + g_test_open_for) {
        open_menu();
    } else if (g_open && t >= g_test_open_at + g_test_open_for) {
        close_menu();
        g_test_done = true;
    }
}

void quit_game() {
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
}

// --- the menu window -----------------------------------------------------------

void draw_stage_select_question(const BwMenuPad& pad, const bw_platform::Look& look) {
    ImGuiIO& io = ImGui::GetIO();
    if (pad.back) {  // B cancels the question, as Esc does
        io.AddKeyEvent(ImGuiKey_Escape, true);
        io.AddKeyEvent(ImGuiKey_Escape, false);
    }
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.94f);
    int answer = 0;
    if (ImGui::Begin("Developers' stage select", nullptr,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::SetWindowFontScale(look.font_scale);
        answer = bw_stage_select_question();
    }
    ImGui::End();
    if (answer != 0)
        close_menu();
}

void draw_menu(const bw_platform::Look& look) {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // The controller, read here and handed to ImGui as keys (menu_shared.h).
    const BwMenuPad pad = bw_menu_controller(env("BLUEWAKE_BUTTON_MAP"), env_on("BLUEWAKE_PAD_SWAP_AB", false));
    bluewake_ui::begin_theme(look.scale);
    if (look.font != nullptr)
        ImGui::PushFont(look.font);
    const auto finish = [&look]() {
        if (look.font != nullptr)
            ImGui::PopFont();
        bluewake_ui::end_theme();
    };
    if (g_stage_select_question) {
        draw_stage_select_question(pad, look);
        finish();
        return;
    }
    // A size of its own rather than its contents', so each tab's body scrolls
    // inside it and the footer stays put: it opens in the middle, and can be
    // moved and resized, but only inside the game's window.
    const ImVec2 display = io.DisplaySize;
    const float scale = look.scale;
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(display.x - 24.f, 900.f * scale), std::min(display.y - 24.f, 720.f * scale)),
                             ImGuiCond_Appearing);
    // Resizing stays between a size that still holds the tabs, hint and buttons
    // and the edges of the game's window, less a margin, from where the menu is.
    const ImVec2 largest(std::max(1.f, display.x - 24.f), std::max(1.f, display.y - 24.f));
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(std::min(440.f * scale, largest.x), std::min(310.f * scale, largest.y)), largest,
        [](ImGuiSizeCallbackData* data) {
            // Growing stops at the edge it is dragged towards: the room right of
            // the left edge (dragging the right one) or left of the right edge
            // (dragging the left one), whichever is larger, as the callback isn't
            // told which; a menu that goes past an edge is moved back in before
            // Begin below. A menu already past it (moved there) isn't shrunk.
            const ImVec2 screen = ImGui::GetIO().DisplaySize;
            const auto limit = [](float desired, float current, float room) {
                return desired <= room ? desired : std::max(room, std::min(current, desired));
            };
            const ImVec2 room(std::max(screen.x - 12.f - data->Pos.x, data->Pos.x + data->CurrentSize.x - 12.f),
                              std::max(screen.y - 12.f - data->Pos.y, data->Pos.y + data->CurrentSize.y - 12.f));
            data->DesiredSize.x = limit(data->DesiredSize.x, data->CurrentSize.x, room.x);
            data->DesiredSize.y = limit(data->DesiredSize.y, data->CurrentSize.y, room.y);
        });
    ImGui::SetNextWindowBgAlpha(0.94f);
    // Dragged, or resized from the top or left, it stays inside the game's
    // window. ImGui moves a dragged window as the frame starts, so this has to
    // come before Begin draws it there; if it is ever bigger than the game's
    // window, the top left (the tabs) stays in view.
    if (const ImGuiWindow* window = ImGui::FindWindowByName("BlueWake settings")) {
        const ImVec2 kept(std::max(12.f, std::min(window->Pos.x, display.x - 12.f - window->Size.x)),
                          std::max(12.f, std::min(window->Pos.y, display.y - 12.f - window->Size.y)));
        if (kept.x != window->Pos.x || kept.y != window->Pos.y)
            ImGui::SetNextWindowPos(kept);
    }
    // A click on the game or outside the menu leaves no window focused, and
    // then the controller has nothing to move through: give the menu focus back.
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
        ImGui::SetNextWindowFocus();
    bool open = true;
    if (ImGui::Begin("BlueWake settings", &open,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::SetWindowFontScale(look.font_scale);
#if defined(_WIN32)
        // Esc closes it in the keyboard hook (bw_settings_key), which keeps the
        // key from SDL; one that reaches ImGui instead closes it too. Not on Mac
        // and Linux: ImGui sees the same Esc that opened the menu (SDL hands it
        // to both), and would close it again that frame.
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            open = false;
#endif
        // The bumpers switch tabs. Each tab's body scrolls on its own but is
        // flattened for navigation, so the D-pad moves from the tabs through the
        // options to the buttons below without A to enter it and B to leave it.
        struct Tab {
            const char* label;
            void (*draw)();
        };
        static const Tab kTabs[] = {{"Display", display_tab},
                                    {"Gameplay", gameplay_tab},
                                    {"Mods", mods_tab},
                                    {"Controls", controls_tab},
                                    {"Sound and files", sound_files_tab}};
        constexpr int kTabCount = static_cast<int>(sizeof kTabs / sizeof kTabs[0]);
        static int shown = 0;
        int request = -1;
        if (pad.tab_step != 0)
            request = (shown + kTabCount + pad.tab_step) % kTabCount;
        // The body takes what the footer leaves: the hint, the next-launch note
        // when there is one, and the buttons on one row or, if narrow, two.
        const bool restart = needs_restart();
        const float footer = bw_menu_footer_height(restart);
        if (ImGui::BeginTabBar("##tabs")) {
            // No title bar, so the mouse's close button sits at the end of the tabs.
            if (ImGui::TabItemButton("x", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                open = false;
            for (int i = 0; i < kTabCount; ++i) {
                if (!ImGui::BeginTabItem(kTabs[i].label, nullptr, i == request ? ImGuiTabItemFlags_SetSelected : 0))
                    continue;
                shown = i;
                ImGui::PushID(i);
                ImGui::BeginChild("##body", ImVec2(0, -footer), ImGuiChildFlags_NavFlattened);
                ImGui::PushTextWrapPos(0.f); // text wraps at the menu's edge
                kTabs[i].draw();
                ImGui::PopTextWrapPos();
                bw_menu_stick_scroll(pad.scroll);
                ImGui::EndChild();
                ImGui::PopID();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        bw_menu_footer_note(restart);
        if (restart && bw_platform::can_restart()) {
            ImGui::SameLine();
            if (ImGui::Button("Restart now")) {
                close_menu();  // saves
                bw_platform::restart();
            }
        }
        switch (bw_menu_footer_buttons(settings_path().c_str())) {
        case BW_MENU_RESUME:
            open = false;
            break;
        // Save states: taken or put back at the game's next clean point once
        // the menu has closed (main.c's host_state_*).
        case BW_MENU_SAVE_STATE:
            bluewake_save_state_hotkey(false);
            open = false;
            break;
        case BW_MENU_LOAD_STATE:
            bluewake_save_state_hotkey(true);
            open = false;
            break;
        case BW_MENU_QUIT:
            close_menu();
            quit_game();
            break;
        case BW_MENU_NONE:
            break;
        }
        // The game's B ends an edit, or closes the menu (menu_shared.h).
        if (bw_menu_back(pad.back))
            open = false;
    }
    ImGui::End();
    finish();
    if (!open)
        close_menu();
}

// Every presented frame, on the main thread, inside Aurora's frame.
void draw(void*) {
    SDL_Window* window = game_window();
    if (window == nullptr)
        return;
    static bool first = true;
    if (first) {
        first = false;
        apply_live();
    }
    if (!bw_platform::ready(window))
        return;
    const Uint64 now = SDL_GetTicks();
    static Uint64 rate_checked;
    if (rate_checked == 0 || now - rate_checked >= 1000) {
        refresh_smooth_rate();  // the display may have changed
        rate_checked = now;
    }
    apply_controller();
    apply_key_map();
    test_hook();
    const bw_platform::Look look = bw_platform::look(window);
    draw_climb_wheel();
    draw_safe_mode_note(now, look);
    draw_shader_wait(look);
    if (bluewake_stage_select_take_question())
        ask_stage_select();
    if (g_open)
        draw_menu(look);
    bw_platform::frame(window, now);
    bluewake_ui::draw_status_line(now, look.scale);
    // A change from a hotkey or the window is written a second after the last one.
    if (!g_open && save_due())
        save();
}

void save_at_exit() { save(); }

}  // namespace

// --- what the platforms call -----------------------------------------------------

void install() {
    g_installed_ms = SDL_GetTicks();
    if (const char* test = std::getenv("BLUEWAKE_SETTINGS_TEST_OPEN"))
        if (std::sscanf(test, "%lf:%lf", &g_test_open_at, &g_test_open_for) != 2)
            g_test_open_at = -1.0;
    // From here on, what a restart would change is measured against this; the
    // aspect, the mods and the pack are added as the menu reads them.
    note_launch();
    g_shader_wait = env_on("BLUEWAKE_SHADERS_FIRST", false);
    dol_aurora_set_overlay(draw, nullptr);
    dol_aurora_set_hold(hold, nullptr);
    dol_aurora_set_hold_redraw(true);
    std::atexit(save_at_exit);
    std::fprintf(stderr, "[settings] Esc (with the mouse free), F1 or a controller's Back opens the options\n");
}

void open_menu() {
    if (g_open)
        return;
    capture_launch_choices();
    g_pack_count.forget();
    g_open = true;
    bluewake_haptics_block(true);
    bw_platform::menu_opened(true);
    std::fprintf(stderr, "[settings] menu open%s\n", g_menu_pauses ? " (the game is paused)" : "");
}

void close_menu() {
    if (!g_open)
        return;
    g_open = false;
    if (g_stage_select_question) {
        g_stage_select_question = false;
        bluewake_stage_select_answer(false); // closed without an answer: cancel
    }
    bluewake_haptics_block(false);
    bw_platform::menu_opened(false);
    bluewake_pad_hold_until_released();
    save();
    std::fprintf(stderr, "[settings] menu closed\n");
}

bool menu_open() { return g_open; }

void ask_stage_select() {
    open_menu();
    g_stage_select_question = g_open;
}

void toggle_hd_look() {
    const bool on = !env_on("BLUEWAKE_HD_LOOK", false);
    set_env("BLUEWAKE_HD_LOOK", on ? "1" : "0");
    bluewake_hd_look_reload();
    bluewake_status_post(on ? "HD look on" : "HD look off");
    std::fprintf(stderr, "[settings] Wind Waker HD look %s (F2)\n", on ? "on" : "off");
}

void toggle_fps() {
    const bool on = !env_on("DOL_AURORA_SHOW_FPS", false);
    set_env("DOL_AURORA_SHOW_FPS", on ? "1" : "0");
    aurora_set_fps_overlay(on);
    bluewake_status_post(on ? "Frame rate on" : "Frame rate off");
}

void toggle_smooth_motion() {
    // On at the rate chosen in the menu (60 if none), or off.
    const bool on = !env_on("DOL_AURORA_FRAME_INTERP", false);
    const std::string steps = env("DOL_AURORA_FRAME_INTERP_STEPS", "1");
    set_smooth_motion(on ? steps.c_str() : nullptr);
    bluewake_status_post(on ? "Smooth Motion on" : "Smooth Motion off");
    std::fprintf(stderr, "[settings] Smooth Motion %s (F10)\n", on ? "on" : "off");
}

void toggle_fullscreen() {
    const bool on = !is_fullscreen();
    set_fullscreen(on);
    bluewake_status_post(on ? "Fullscreen on" : "Fullscreen off");
}

SDL_Window* game_window() {
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);
    SDL_Window* window = windows != nullptr && count > 0 ? windows[0] : nullptr;
    SDL_free(windows);
    return window;
}

}  // namespace bw_desktop
