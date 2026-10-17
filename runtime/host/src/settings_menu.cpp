// The options menu (the Mac host): the settings the host reads from the
// environment, in a window over the paused game, saved to a settings file.
#include "settings_menu.h"
#include "smooth_rate.h"
#include "fps_position.h"
#include "controller_face_swap.h"
#include "atomic_file.h"
#include "launch_marker.h"

// The host's modules are C.
extern "C" {
#include "climb.h"
#include "dpad_menus.h"
#include "fast_load.h"
#include "game_options.h"
#include "forest_water.h"
#include "hd_look.h"
#include "haptics.h"
#include "jump_button.h"
#include "mouse_camera.h"
#include "quick_doors.h"
#include "save_state.h"
#include "sprint.h"
}

#include "gxruntime/aurora_backend.h"
#include "desktop_theme.h"
#include "status_line_overlay.h"

#include <SDL3/SDL.h>
#include <aurora/aurora.h>
#include <aurora/imgui.h>
#include <aurora/gfx.h>
#include <imgui.h>
#include <imgui_internal.h> // FindWindowByName: the menu's position before Begin draws it
#include "button_remap.h"
#include "menu_shared.h"
#include "input_remap.h"
#include "option_notes.h"
#include "stage_select_key.h"
#include "texture_pack_menu.h"

#include <algorithm>
#include <cmath>

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <sys/stat.h>
#include <vector>

extern "C" {
void aurora_set_frame_buffer_scale(float scale);
void aurora_set_frame_interpolation(bool enabled);
void aurora_set_frame_interp_steps(int steps);
void aurora_set_fps_overlay(bool enabled);
void aurora_set_forced_anisotropy(unsigned samples);
}

namespace {

// The settings the menu writes, in the file's order. The rest of the file (keys
// the menu does not show) is kept as it was.
const char* const kKeys[] = {
    "BLUEWAKE_ASPECT",          "DOL_AURORA_FULLSCREEN",    "DOL_AURORA_RENDER_SCALE",
    "DOL_AURORA_FRAME_INTERP",  "DOL_AURORA_FRAME_INTERP_STEPS", "DOL_AURORA_SHOW_FPS", "DOL_AURORA_FPS_POSITION",
    "DOL_AURORA_FORCE_ANISO",
    "DOL_AURORA_TEXTURE_PACK",  "BLUEWAKE_MODS",            "BLUEWAKE_OPTIONS",
    "BLUEWAKE_FADE_FRAMES",     "BLUEWAKE_FAST_FORWARD",    "BLUEWAKE_QUICK_DOORS",
    "BLUEWAKE_JUMP_BUTTON", "BLUEWAKE_PAD_SWAP_AB", "BLUEWAKE_PAD_SWAP_XY", "BLUEWAKE_PAD_SWAP_LR",
    "BLUEWAKE_DPAD_MENUS",
    "BLUEWAKE_BUTTON_MAP",
    "BLUEWAKE_SPRINT_SPEED",    "BLUEWAKE_MOUSE_CAMERA",    "BLUEWAKE_MOUSE_SENSITIVITY",
    "BLUEWAKE_MOUSE_INVERT_Y",  "BLUEWAKE_MOUSE_BUTTONS",   "BLUEWAKE_KEY_MAP",
    "BLUEWAKE_STICK_CAMERA",    "BLUEWAKE_STICK_CAMERA_SPEED",
    "BLUEWAKE_STICK_CAMERA_INVERT_X", "BLUEWAKE_STICK_CAMERA_INVERT_Y", "BLUEWAKE_STICK_AIM_SPEED",
    "BLUEWAKE_STICK_ZOOM",      "BLUEWAKE_STICK_ZOOM_SPEED", "BLUEWAKE_AIM_INVERT_Y",
    "BLUEWAKE_GYRO_AIM",        "BLUEWAKE_GYRO_AIM_SPEED",
    "BLUEWAKE_HAPTICS", "BLUEWAKE_HAPTICS_STRENGTH", "BLUEWAKE_HAPTICS_TRIGGERS",
    "BLUEWAKE_CLIMB",           "BLUEWAKE_CLIMB_STAMINA",
    "BLUEWAKE_FOREST_WATER_KEEP_TREES", "BLUEWAKE_FOREST_WATER_30_MINUTES",
    "BLUEWAKE_STAGE_SELECT_TOGGLE", "BLUEWAKE_HD_LOOK",
    "DOL_AURORA_ASPECT_FIT",    "BLUEWAKE_PAUSE_UNFOCUSED", "BLUEWAKE_SHADERS_FIRST",
    "BLUEWAKE_SPRINT_CHOICE",   "BLUEWAKE_DSP_MODE",
};

std::string g_path;                          // the settings file ("" when none)
std::string g_data_dir;                      // saves and logs (bluewake_settings_set_data_dir)
std::map<std::string, std::string> g_other;  // its other keys, kept as they were
bool g_open = false;
bool g_stage_select_question = false; // the menu shows only the stage select's question
bool g_dirty = false;
bool g_nav_set = false;

// Settings that take effect at the next launch, as chosen now.
std::string g_aspect;
bool g_betterww = false;
std::vector<std::pair<std::string, bool>> g_options; // name, on
std::vector<std::string> g_option_titles;
char g_texture_pack[1024];
std::string g_last_pack;             // the folder before HD textures went off (this session)
bw_texture_pack::Counter g_pack_count; // textures in g_texture_pack, counted once per menu opening
bool g_restart_pending = false;
bool g_restart_requested = false;  // Restart now: linux_entry.c starts the game again
bool g_safe_mode = false;          // this launch is in safe mode (BLUEWAKE_SAFE_MODE)
bool g_safe_backed_up = false;     // and the settings file was copied aside first
Uint64 g_safe_mode_since = 0;
// BLUEWAKE_SETTINGS_TEST_OPEN=at:for (seconds after the menu is installed;
// testing only): opens the menu, and closes it after `for` seconds.
double g_test_open_at = -1.0, g_test_open_for = 0.0;
Uint64 g_installed_ms = 0;
bool g_test_done = false;

std::string env(const char* key, const char* fallback = "") {
    const char* value = std::getenv(key);
    return value != nullptr ? value : fallback;
}

bool env_on(const char* key, bool fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0')
        return fallback;
    return value[0] != '0';
}

void set_env(const char* key, const std::string& value) {
    setenv(key, value.c_str(), 1);
    g_dirty = true;
}

std::string default_path() {
    const char* home = std::getenv("HOME");
    if (home == nullptr || home[0] == '\0')
        return "";
#if defined(__linux__)
    // Freedesktop config dir: $XDG_CONFIG_HOME/BlueWake, else ~/.config/BlueWake.
    const char* config = std::getenv("XDG_CONFIG_HOME");
    if (config != nullptr && config[0] != '\0')
        return std::string(config) + "/BlueWake/settings.ini";
    return std::string(home) + "/.config/BlueWake/settings.ini";
#else
    return std::string(home) + "/Library/Application Support/BlueWake/settings.ini";
#endif
}

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool managed(const std::string& key) {
    for (const char* k : kKeys)
        if (key == k)
            return true;
    return false;
}

// Better Wind Waker as a mod the user chose: BLUEWAKE_MODS without the
// widescreen mod, which BLUEWAKE_ASPECT adds at every launch.
std::string chosen_mods() { return g_betterww ? "betterww" : ""; }

std::string options_value() {
    std::string list = "none";
    for (const auto& [name, on] : g_options)
        if (on)
            list += "," + name;
    return list;
}

void capture_launch_choices();

void save() {
    if (g_path.empty() || !g_dirty)
        return;
    // The aspect, Better Wind Waker and the texture pack are written from the
    // menu's copies, read when it first opens: a key (F2, F9, F10, F11) that
    // saves before then would otherwise write them empty.
    capture_launch_choices();
    const auto slash = g_path.rfind('/');
    if (slash != std::string::npos) {
        // Application Support/BlueWake, one level at a time.
        std::string dir = g_path.substr(0, slash);
        for (size_t at = 1; (at = dir.find('/', at)) != std::string::npos; ++at)
            mkdir(dir.substr(0, at).c_str(), 0755);
        mkdir(dir.c_str(), 0755);
    }
    char* pending = bw_atomic_path(g_path.c_str());
    FILE* file = pending != nullptr ? std::fopen(pending, "w") : nullptr;
    if (file == nullptr) {
        std::fprintf(stderr, "[settings] cannot write %s: %s\n", g_path.c_str(), std::strerror(errno));
        free(pending);
        return;
    }
    std::fputs("# BlueWake settings, written by the options menu (Esc or F1 in the game).\n"
               "# KEY=VALUE, the host's environment settings; these win over the launch command's.\n",
               file);
    for (const char* key : kKeys) {
        std::string value;
        if (std::strcmp(key, "BLUEWAKE_ASPECT") == 0)
            value = g_aspect;
        else if (std::strcmp(key, "BLUEWAKE_MODS") == 0)
            value = chosen_mods();
        else if (std::strcmp(key, "BLUEWAKE_OPTIONS") == 0)
            value = g_options.empty() ? env(key) : options_value();
        else if (std::strcmp(key, "DOL_AURORA_TEXTURE_PACK") == 0)
            value = g_texture_pack;
        else
            value = env(key);
        std::fprintf(file, "%s=%s\n", key, value.c_str());
    }
    for (const auto& [key, value] : g_other)
        std::fprintf(file, "%s=%s\n", key.c_str(), value.c_str());
    bool ok = bw_atomic_finish_dirty(file, pending, g_path.c_str(), &g_dirty);
    free(pending);
    std::fprintf(stderr, "[settings] %s %s\n", ok ? "saved" : "save failed; retry pending for", g_path.c_str());
}

// The launch-time choices (the environment after the launch and the settings
// file, and the options the game started with), read the first time the menu
// opens. Later openings keep what the player chose since: reading them again
// would show, and then save, the launch values over the player's changes.
void capture_launch_choices() {
    static bool captured = false;
    if (!captured) {
        captured = true;
        g_aspect = env("BLUEWAKE_ASPECT", "4:3");
        const std::string mods = env("BLUEWAKE_MODS");
        g_betterww = mods.find("betterww") != std::string::npos;
        std::snprintf(g_texture_pack, sizeof g_texture_pack, "%s", env("DOL_AURORA_TEXTURE_PACK").c_str());
    }
    // Better Wind Waker's options are listed once the game has started.
    if (!g_options.empty())
        return;
    g_option_titles.clear();
    for (u32 i = 0;; ++i) {
        const char* title = nullptr;
        bool default_on = false, on = false;
        const char* name = bluewake_game_options_describe(i, &title, &default_on, &on);
        if (name == nullptr)
            break;
        g_options.emplace_back(name, g_betterww ? on : default_on);
        g_option_titles.emplace_back(title != nullptr ? title : name);
    }
}

SDL_Window* game_window() {
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);
    SDL_Window* window = windows != nullptr && count > 0 ? windows[0] : nullptr;
    SDL_free(windows);
    return window;
}

void open_menu() {
    if (g_open)
        return;
    bluewake_mouse_camera_release();
    bluewake_haptics_block(true);
    capture_launch_choices();
    g_pack_count.forget();
    g_open = true;
    std::fprintf(stderr, "[settings] menu open (the game is paused)\n");
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
    bluewake_pad_hold_until_released();
    save();
    std::fprintf(stderr, "[settings] menu closed\n");
}

// Compile shaders before playing (BLUEWAKE_SHADERS_FIRST=1), as on Windows: at
// start the game waits until the shaders from earlier play are ready, at most
// two minutes, with a progress box.
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

// Safe mode's note, for the first fifteen seconds, as on Windows.
void draw_safe_mode_note(Uint64 now) {
    if (!g_safe_mode)
        return;
    if (g_safe_mode_since == 0)
        g_safe_mode_since = now;
    if (now - g_safe_mode_since > 15000)
        return;
    ImGui::SetNextWindowBgAlpha(0.85f);
    if (ImGui::Begin("Launch recovery", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
        // A fixed wrap width: in an auto-resizing window, wrapping to the
        // window's own width collapses it to a narrow column.
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.f);
        ImGui::TextUnformatted(g_safe_backed_up
                                   ? "BlueWake didn't finish starting last time, so this time it starts with Fast "
                                     "sound, 4:3, in a window and with the mods off. Your earlier settings were saved "
                                     "beside settings.ini. You can turn things back on in the settings (F1)."
                                   : "BlueWake didn't finish starting last time, so this time it starts with Fast "
                                     "sound, 4:3, in a window and with the mods off, for this session only: your "
                                     "settings come back at the next launch.");
        ImGui::PopTextWrapPos();
    }
    ImGui::End();
}

void draw_shader_wait() {
    if (!g_shader_wait)
        return;
    const AuroraStats* stats = aurora_get_stats();
    const uint32_t left = stats != nullptr ? stats->queuedPipelines : 0u;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.75f);
    if (ImGui::Begin("##bluewake-shaders", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav)) {
        ImGui::Text("Compiling shaders: %u to go", left);
        if (g_shader_wait_first > 0u)
            ImGui::ProgressBar(1.f - static_cast<float>(left) / static_cast<float>(g_shader_wait_first),
                               ImVec2(260.f, 0.f), "");
    }
    ImGui::End();
}

// The game waits while the menu is open or the shaders are compiling.
bool hold(void*) { return g_open || shader_wait_holds(); }

// A setting that the game takes at the next launch.
void restart_note() {
    ImGui::SameLine();
    ImGui::TextDisabled("(next launch)");
}

bool slider(const char* label, float* value, float low, float high, const char* format) {
    ImGui::PushID(label);
    ImGui::TextWrapped("%s", label);
    ImGui::SetNextItemWidth(-1.f);
    bool changed = ImGui::SliderFloat("##value", value, low, high, format);
    if (const int step = bw_menu_slider_nudge()) {
        *value = std::clamp(*value + step * (high - low) / 20.f, low, high);
        changed = true;
    }
    ImGui::PopID();
    return changed;
}

void refresh_smooth_rate() {
    SDL_Window* window = game_window();
    const SDL_DisplayMode* mode = window != nullptr ? SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window)) : nullptr;
    const int requested = bw_smooth_requested(env("DOL_AURORA_FRAME_INTERP_STEPS", "1").c_str());
    aurora_set_frame_interp_steps(bw_smooth_steps(requested, mode != nullptr ? mode->refresh_rate : 0.f));
}

// Fullscreen, the frame rate and Smooth Motion, from the Display tab or their
// keys (F11 or Alt+Enter, F9, F10), the same keys as on Windows (#320).
// Fullscreen here is for this session only; whether the game starts in
// fullscreen is its own setting (DOL_AURORA_FULLSCREEN, read at launch), so
// trying fullscreen never changes how the game starts.
bool is_fullscreen() {
    SDL_Window* window = game_window();
    return window != nullptr && (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
}

void set_fullscreen(bool on) {
    if (SDL_Window* window = game_window())
        SDL_SetWindowFullscreen(window, on);
}

void set_show_fps(bool on) {
    set_env("DOL_AURORA_SHOW_FPS", on ? "1" : "0");
    aurora_set_fps_overlay(on);
}

// steps: "1", "3" or "display"; nullptr turns it off.
void set_smooth_motion(const char* steps) {
    set_env("DOL_AURORA_FRAME_INTERP", steps != nullptr ? "1" : "0");
    if (steps != nullptr)
        set_env("DOL_AURORA_FRAME_INTERP_STEPS", steps);
    refresh_smooth_rate();
    aurora_set_frame_interpolation(steps != nullptr);
}

// Out of fullscreen, at the size the window starts at (DOL_AURORA_WINDOW, or
// 720 tall in the picture's shape), in the middle of the screen.
void reset_window() {
    SDL_Window* window = game_window();
    if (window == nullptr)
        return;
    set_fullscreen(false);
    int width = 0, height = 0;
    const std::string chosen = env("DOL_AURORA_WINDOW");
    if (std::sscanf(chosen.c_str(), "%dx%d", &width, &height) != 2 || width < 320 || height < 240) {
        const float ratio = std::strtof(env("DOL_AURORA_ASPECT_RATIO", "0").c_str(), nullptr);
        height = 720;
        width = static_cast<int>(height * (ratio > 1.f && ratio < 4.f ? ratio : 4.f / 3.f) + 0.5f);
    }
    SDL_SetWindowSize(window, width, height);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void display_tab() {
    bool hd_look = env_on("BLUEWAKE_HD_LOOK", false);
    if (bw_menu_checkbox("Wind Waker HD look   (F2)", &hd_look)) {
        set_env("BLUEWAKE_HD_LOOK", hd_look ? "1" : "0");
        bluewake_hd_look_reload();
        g_dirty = true;
    }
    ImGui::TextWrapped("The HD release's look, rebuilt piece by piece. So far: islands on the horizon from "
                       "further away, soft shading, cloud shadows and edge light outdoors, torch light, a soft distance "
                       "and bloom. Off by default.");

    // An action for this session, not a setting: a button.
    const bool fullscreen = is_fullscreen();
    if (ImGui::Button(fullscreen ? "Leave fullscreen   (F11 or Alt+Enter)" : "Fullscreen   (F11 or Alt+Enter)"))
        set_fullscreen(!fullscreen);
    bool start_fullscreen = env_on("DOL_AURORA_FULLSCREEN", false);
    if (bw_menu_checkbox("Start in fullscreen", &start_fullscreen)) {
        set_env("DOL_AURORA_FULLSCREEN", start_fullscreen ? "1" : "0");
        g_dirty = true;
    }
    restart_note();

    static const char* const kScales[] = {"The window's pixels", "1x (480 lines)", "2x (960)", "3x (1440)",
                                          "4x (1920)"};
    int scale = std::atoi(env("DOL_AURORA_RENDER_SCALE", "0").c_str());
    scale = scale < 0 ? 0 : scale > 4 ? 4 : scale;
    if (bw_menu_combo("Render resolution", &scale, kScales, 5)) {
        set_env("DOL_AURORA_RENDER_SCALE", std::to_string(scale));
        aurora_set_frame_buffer_scale(static_cast<float>(scale));
    }

    static const char* const kSmooth[] = {"Off (30, the game's)", "60 frames a second",
        "120 frames a second (120 Hz displays)", "Match the display (up to 240)"};
    const int requested = bw_smooth_requested(env("DOL_AURORA_FRAME_INTERP_STEPS", "1").c_str());
    int smooth = !env_on("DOL_AURORA_FRAME_INTERP", false) ? 0 : requested == -1 ? 3 : requested >= 3 ? 2 : 1;
    if (bw_menu_combo("Smooth Motion (experimental)   (F10)", &smooth, kSmooth, 4))
        set_smooth_motion(smooth == 0 ? nullptr : smooth == 3 ? "display" : smooth == 2 ? "3" : "1");
    ImGui::TextDisabled("The game runs at 30; display changes and overloads can lower the presentation rate.");

    bool fps = env_on("DOL_AURORA_SHOW_FPS", false);
    if (bw_menu_checkbox("Show the frame rate   (F9)", &fps))
        set_show_fps(fps);
    ImGui::BeginDisabled(!fps);
    int position = bw_fps_position(env("DOL_AURORA_FPS_POSITION").c_str());
    if (bw_menu_combo("FPS position", &position, kBwFpsPositionNames, BW_FPS_POSITIONS)) {
        set_env("DOL_AURORA_FPS_POSITION", kBwFpsPositionValues[position]);
        aurora_set_fps_overlay_position(static_cast<AuroraFpsOverlayPosition>(position));
    }
    ImGui::EndDisabled();

    static const char* const kAniso[] = {"Off (the game's)", "2x", "4x", "8x", "16x"};
    static const unsigned kAnisoValues[] = {1, 2, 4, 8, 16};
    const unsigned samples = static_cast<unsigned>(std::atoi(env("DOL_AURORA_FORCE_ANISO", "1").c_str()));
    int aniso = 0;
    for (int i = 0; i < 5; ++i)
        if (samples == kAnisoValues[i])
            aniso = i;
    if (bw_menu_combo("Texture filtering", &aniso, kAniso, 5)) {
        set_env("DOL_AURORA_FORCE_ANISO", std::to_string(kAnisoValues[aniso]));
        aurora_set_forced_anisotropy(kAnisoValues[aniso]);
    }

    // When the window isn't the picture's shape: black bars, or the picture
    // stretched to fill it. A widened picture (16:10, 16:9, 21:9) always keeps
    // its shape, so the choice is only for 4:3.
    if (g_aspect == "4:3") {
        bool keep_shape = env_on("DOL_AURORA_ASPECT_FIT", false);
        if (bw_menu_checkbox("Keep the picture's shape (black bars rather than stretching)", &keep_shape)) {
            set_env("DOL_AURORA_ASPECT_FIT", keep_shape ? "1" : "0");
            g_dirty = g_restart_pending = true;
        }
        restart_note();
    } else {
        ImGui::TextDisabled("Widened pictures always keep their shape (black bars rather than stretching).");
    }

    bool pause = env_on("BLUEWAKE_PAUSE_UNFOCUSED", false);
    if (bw_menu_checkbox("Pause while the window is in the background", &pause)) {
        set_env("BLUEWAKE_PAUSE_UNFOCUSED", pause ? "1" : "0");
        aurora_set_pause_on_focus_lost(pause);
    }

    bool shaders_first = env_on("BLUEWAKE_SHADERS_FIRST", false);
    if (bw_menu_checkbox("Compile shaders before playing", &shaders_first)) {
        set_env("BLUEWAKE_SHADERS_FIRST", shaders_first ? "1" : "0");
        g_dirty = g_restart_pending = true;
    }
    restart_note();
    ImGui::TextDisabled("At start, wait until the shaders from earlier play are ready, so nothing is missing from "
                        "the picture the first time it is drawn. Otherwise they are made while you play.");

    if (ImGui::Button("Reset the window"))
        reset_window();
    ImGui::SameLine();
    ImGui::TextDisabled("Out of fullscreen, at its starting size, in the middle of the screen.");

}

// The folder scripts/import_wwhd_textures.py puts its pack in.
std::string importer_pack_folder() {
    // Follow the app's data/settings location, including Linux portable mode
    // and isolated sessions. The normal Mac/Linux locations stay the same.
    std::filesystem::path base = g_data_dir;
    if (base.empty() && !g_path.empty())
        base = std::filesystem::path(g_path).parent_path();
    if (!base.empty())
        return (base / "Load" / "Textures" / "WWHD").string();
    const char* home = std::getenv("HOME");
    if (home == nullptr || home[0] == '\0')
        return "";
#if defined(__linux__)
    const char* data = std::getenv("XDG_DATA_HOME");
    const std::string fallback = data != nullptr && data[0] != '\0' ? data : std::string(home) + "/.local/share";
    return fallback + "/BlueWake/Load/Textures/WWHD";
#else
    return std::string(home) + "/Library/Application Support/BlueWake/Load/Textures/WWHD";
#endif
}


void set_texture_pack(const std::string& folder) {
    std::snprintf(g_texture_pack, sizeof g_texture_pack, "%s", folder.c_str());
    g_dirty = g_restart_pending = true;
}

// Wind Waker HD textures: the same section, in the same order, as the Windows
// Mods tab and the iPhone and iPad Mods menu. The pack is on while a folder is
// chosen (DOL_AURORA_TEXTURE_PACK), as in the Linux setup window.
void hd_textures_section() {
    std::string picked;
    if (bw_texture_pack::take_folder(picked))
        set_texture_pack(picked);
    ImGui::SeparatorText("Wind Waker HD textures");
    bool on = g_texture_pack[0] != '\0';
    if (bw_menu_checkbox("HD textures", &on)) {
        if (!on) {
            g_last_pack = g_texture_pack;
            set_texture_pack("");
        } else {
            std::error_code ec;
            const std::string folder = !g_last_pack.empty() ? g_last_pack
                : std::filesystem::is_directory(importer_pack_folder(), ec) ? importer_pack_folder() : "";
            if (folder.empty())
                bw_texture_pack::choose_folder(game_window(), "");
            else
                set_texture_pack(folder);
        }
    }
    restart_note();
    {
        const size_t textures = g_pack_count.get(g_texture_pack);
        const bw_texture_pack::PackState state =
            bw_texture_pack::state_of(textures, g_texture_pack[0] != '\0');
        std::string line;
        if (g_texture_pack[0] == '\0')
            line = "Off. A Dolphin-format pack for GZLE01 (a folder of .png or .dds files) replaces the game's textures.";
        else if (textures == 0)
            line = "No .png or .dds textures found in the chosen folder.";
        else
            line = std::to_string(textures) + " textures in the chosen folder";
        ImGui::TextColored(bw_texture_pack::state_color(state), "%s", line.c_str());
    }
    if (ImGui::Button("Choose texture pack folder..."))
        bw_texture_pack::choose_folder(game_window(), g_texture_pack);
    std::string imported;
    const std::string textures = std::filesystem::path(importer_pack_folder()).parent_path().string();
    if (bw_texture_pack::import_controls(game_window(), textures, imported))
        set_texture_pack(imported);
    const std::string error = bw_texture_pack::picker_error();
    if (!error.empty()) {
        ImGui::TextWrapped("The folder picker did not open (%s). Type the folder instead:", error.c_str());
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::InputText("##texpack", g_texture_pack, sizeof g_texture_pack))
            g_dirty = g_restart_pending = true;
    }
}

// Mods: HD textures first, then widescreen and Better Wind Waker, as in the
// Windows Mods tab and the iPhone and iPad Mods menu.
void mods_tab() {
    hd_textures_section();
    ImGui::Spacing();
    static const char* const kAspects[] = {"Off (4:3, the game's)", "16:10", "16:9", "21:9"};
    static const char* const kAspectValues[] = {"4:3", "16:10", "16:9", "21:9"};
    int aspect = 0;
    for (int i = 0; i < 4; ++i)
        if (g_aspect == kAspectValues[i])
            aspect = i;
    if (bw_menu_combo("Widescreen", &aspect, kAspects, 4)) {
        g_aspect = kAspectValues[aspect];
        g_dirty = g_restart_pending = true;
    }
    restart_note();
    ImGui::TextDisabled("Widescreen widens the camera and moves the HUD to the edges.");
    ImGui::Spacing();
    if (bw_menu_checkbox("Better Wind Waker", &g_betterww))
        g_dirty = g_restart_pending = true;
    restart_note();
    if (g_options.empty()) {
        ImGui::TextDisabled("Its settings are listed once the game has started.");
    } else {
        ImGui::BeginDisabled(!g_betterww);
        ImGui::Indent();
        for (size_t i = 0; i < g_options.size(); ++i) {
            bool on = g_options[i].second;
            if (bw_menu_checkbox(g_option_titles[i].c_str(), &on)) {
                g_options[i].second = on;
                g_dirty = g_restart_pending = true;
            }
            if (const char* note = bw_option_note(g_options[i].first.c_str())) {
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
                ImGui::TextDisabled("%s", note);
                ImGui::PopTextWrapPos();
            }
        }
        ImGui::Unindent();
        ImGui::EndDisabled();
    }


}

void gameplay_tab() {
    bool stage_select = env_on("BLUEWAKE_STAGE_SELECT_TOGGLE", false);
    if (bw_menu_checkbox("Developers' stage select (F7)", &stage_select)) {
        set_env("BLUEWAKE_STAGE_SELECT_TOGGLE", stage_select ? "1" : "0");
        g_dirty = g_restart_pending = true;
    }
    restart_note();
    ImGui::TextWrapped("F7 during play opens the menu the developers used to jump to any room, from your own "
                       "file; F7 in the menu goes back. On a controller, Back (View on Xbox, Share or Create on "
                       "PlayStation) opens these settings and the button below does the same. Off by default.");
    bw_stage_select_save_warning();
    if (bw_stage_select_button())
        close_menu();

    ImGui::SeparatorText("Forest Water Challenge");
    bool keep_trees = env_on("BLUEWAKE_FOREST_WATER_KEEP_TREES", false);
    if (bw_menu_checkbox("Keep watered trees when time runs out", &keep_trees)) {
        set_env("BLUEWAKE_FOREST_WATER_KEEP_TREES", keep_trees ? "1" : "0");
        bluewake_forest_water_reload();
    }
    ImGui::TextWrapped("Forest Water still expires. Refill and continue with the remaining trees.");
    bool thirty_minutes = env_on("BLUEWAKE_FOREST_WATER_30_MINUTES", false);
    if (bw_menu_checkbox("30-minute Forest Water timer", &thirty_minutes)) {
        set_env("BLUEWAKE_FOREST_WATER_30_MINUTES", thirty_minutes ? "1" : "0");
        bluewake_forest_water_reload();
    }
    ImGui::TextWrapped("Applies the next time you collect Forest Water. Both options are off by default.");

    ImGui::SeparatorText("Doors and exits");
    // One switch, as on Windows and the iPad: short fades and no waiting in
    // the black, or the game's own (BLUEWAKE_FADE_FRAMES 6 or 0, and
    // BLUEWAKE_FAST_FORWARD, which keep working on their own for testing).
    bool fast = env_on("BLUEWAKE_FAST_FORWARD", true);
    if (bw_menu_checkbox("Fast transitions (short fades, no waiting in the black)", &fast)) {
        set_env("BLUEWAKE_FAST_FORWARD", fast ? "1" : "0");
        set_env("BLUEWAKE_FADE_FRAMES", fast ? "6" : "0");
        bluewake_fast_load_reload();
    }
    bool doors = env_on("BLUEWAKE_QUICK_DOORS", true);
    if (bw_menu_checkbox("Quick doors (no walk-in or door closing behind Link)", &doors)) {
        set_env("BLUEWAKE_QUICK_DOORS", doors ? "1" : "0");
        bluewake_quick_doors_reload();
    }

    ImGui::SeparatorText("Climbing");
    bool climb = env_on("BLUEWAKE_CLIMB", false);
    if (bw_menu_checkbox("Wall climbing (experimental)", &climb)) {
        set_env("BLUEWAKE_CLIMB", climb ? "1" : "0");
        bluewake_climb_reload();
    }
    ImGui::BeginDisabled(!climb);
    float stamina = static_cast<float>(std::atof(env("BLUEWAKE_CLIMB_STAMINA", "12").c_str()));
    if (stamina < 1.f)
        stamina = 12.f;
    if (slider("Climbing stamina", &stamina, 4.f, 30.f, "%.0f seconds")) {
        char text[16];
        std::snprintf(text, sizeof text, "%.0f", stamina);
        set_env("BLUEWAKE_CLIMB_STAMINA", text);
        bluewake_climb_reload();
    }
    ImGui::EndDisabled();

    ImGui::SeparatorText("Jump & Sprint");
    // Jump & Sprint, one switch as on Windows and the iPad, with the sprint's
    // speed under it. The chosen speed is kept apart (BLUEWAKE_SPRINT_CHOICE) so
    // turning it off (BLUEWAKE_SPRINT_SPEED 1) and on again keeps it.
    const float current = static_cast<float>(std::atof(env("BLUEWAKE_SPRINT_SPEED", "1").c_str()));
    bool extras = env_on("BLUEWAKE_JUMP_BUTTON", false) || current > 1.01f;
    float choice = static_cast<float>(std::atof(env("BLUEWAKE_SPRINT_CHOICE", "0").c_str()));
    if (choice < 1.1f || choice > 2.f)
        choice = current > 1.01f && current <= 2.f ? std::max(current, 1.1f) : 1.5f;
    const auto apply_sprint = [&]() {
        char text[16];
        std::snprintf(text, sizeof text, "%.2f", choice);
        set_env("BLUEWAKE_SPRINT_CHOICE", text);
        set_env("BLUEWAKE_SPRINT_SPEED", extras ? text : "1");
        set_env("BLUEWAKE_JUMP_BUTTON", extras ? "1" : "0");
        bluewake_jump_button_reload();
        bluewake_sprint_reload();
    };
    if (bw_menu_checkbox("Jump & Sprint (Space or the left bumper jumps; Shift or the left stick click sprints)", &extras))
        apply_sprint();
    ImGui::BeginDisabled(!extras);
    if (slider("Sprint speed", &choice, 1.1f, 2.f, "%.2fx"))
        apply_sprint();
    ImGui::EndDisabled();
}

void apply_controller_swaps() {
    static SDL_JoystickID previous = 0;
    static bool applied = false, last_ab = false, last_xy = false, last_lr = false, last_ix = false, last_iy = false;
    static std::string last_map;
    int index = PADGetIndexForPort(0);
    SDL_JoystickID connection = bw_controller_connection(0);
    bool ab = env_on("BLUEWAKE_PAD_SWAP_AB", false), xy = env_on("BLUEWAKE_PAD_SWAP_XY", false);
    const bool lr = env_on("BLUEWAKE_PAD_SWAP_LR", false);
    // The right stick's inversion reaches the game's own C-stick too, as on
    // Windows, while the direct stick camera is on (otherwise the game's stick
    // follows Better Wind Waker's "Invert camera", as the Controls tab says).
    const bool stick = env_on("BLUEWAKE_STICK_CAMERA", true);
    const bool ix = stick && env_on("BLUEWAKE_STICK_CAMERA_INVERT_X", false);
    const bool iy = stick && env_on("BLUEWAKE_STICK_CAMERA_INVERT_Y", false);
    const std::string map_text = env("BLUEWAKE_BUTTON_MAP");
    if (connection == previous && ab == last_ab && xy == last_xy && lr == last_lr && ix == last_ix &&
        iy == last_iy && map_text == last_map)
        return;
    previous = connection; last_ab = ab; last_xy = xy; last_lr = lr; last_ix = ix; last_iy = iy;
    last_map = map_text;
    BwButtonMap map;
    const bool remapped = bw_button_map_parse(map_text, &map);
    if (index < 0 || (!ab && !xy && !lr && !ix && !iy && !remapped && !applied)) return;
    PADRestoreDefaultMapping(0);
    // A custom layout replaces the swaps; otherwise the swaps as before.
    if (remapped)
        bw_apply_button_map(0, map);
    else
        bw_apply_face_swaps(0, ab, xy);
    bw_apply_camera_axes(0, ix, iy);
    bw_apply_trigger_swap(0, lr);
    applied = true;
}

// The keyboard's keys for the GameCube buttons (BLUEWAKE_KEY_MAP), applied once
// the pad's keyboard bindings exist and again whenever they change.
void apply_key_map() {
    static bool applied = false;
    static std::string last;
    const std::string text = env("BLUEWAKE_KEY_MAP");
    if (applied && text == last) return;
    BwKeyMap map;
    bw_key_map_parse(text, &map);
    if (bw_apply_key_map(0, map)) {
        applied = true;
        last = text;
    }
}

void controls_tab() {
    BwButtonMap map;
    const bool remapped = bw_button_map_parse(env("BLUEWAKE_BUTTON_MAP"), &map);
    bool swap_ab = env_on("BLUEWAKE_PAD_SWAP_AB", false), swap_xy = env_on("BLUEWAKE_PAD_SWAP_XY", false);
    ImGui::BeginDisabled(remapped);
    if (bw_menu_checkbox("Swap A and B", &swap_ab)) set_env("BLUEWAKE_PAD_SWAP_AB", swap_ab ? "1" : "0");
    if (bw_menu_checkbox("Swap X and Y", &swap_xy)) set_env("BLUEWAKE_PAD_SWAP_XY", swap_xy ? "1" : "0");
    ImGui::EndDisabled();
    bool swap_lr = env_on("BLUEWAKE_PAD_SWAP_LR", false);
    if (bw_menu_checkbox("Swap L and R (the triggers)", &swap_lr)) set_env("BLUEWAKE_PAD_SWAP_LR", swap_lr ? "1" : "0");
    bool dpad_menus = env_on("BLUEWAKE_DPAD_MENUS", false);
    if (bw_menu_checkbox("D-pad moves the cursor in Items and Quest Status", &dpad_menus)) {
        set_env("BLUEWAKE_DPAD_MENUS", dpad_menus ? "1" : "0");
        bluewake_dpad_menus_reload();
    }
    if (ImGui::CollapsingHeader("Controller buttons")) {
        ImGui::TextWrapped("Choose which controller button presses each GameCube button. Picking one that is "
                           "already used swaps the two.%s", remapped ? " The swaps above are off while you use "
                           "a custom layout." : "");
        if (bw_button_map_ui(&map))
            set_env("BLUEWAKE_BUTTON_MAP", bw_button_map_format(map));
    }
    if (ImGui::CollapsingHeader("Keyboard keys")) {
        BwKeyMap keys;
        bw_key_map_parse(env("BLUEWAKE_KEY_MAP"), &keys);
        ImGui::TextWrapped("Choose the key for each GameCube button. Picking one that is already used swaps the two.");
        if (bw_key_map_ui(&keys))
            set_env("BLUEWAKE_KEY_MAP", bw_key_map_format(keys));
    }
    apply_controller_swaps();
    bool mouse = env_on("BLUEWAKE_MOUSE_CAMERA", true);
    if (bw_menu_checkbox("Mouse camera (click the game to use it)", &mouse)) {
        set_env("BLUEWAKE_MOUSE_CAMERA", mouse ? "1" : "0");
        bluewake_mouse_camera_reload();
    }
    ImGui::BeginDisabled(!mouse);
    float sensitivity = static_cast<float>(std::atof(env("BLUEWAKE_MOUSE_SENSITIVITY", "1.0").c_str()));
    if (sensitivity <= 0.f)
        sensitivity = 1.f;
    if (slider("Mouse sensitivity", &sensitivity, 0.2f, 3.f, "%.2f")) {
        char text[16];
        std::snprintf(text, sizeof text, "%.2f", sensitivity);
        set_env("BLUEWAKE_MOUSE_SENSITIVITY", text);
        bluewake_mouse_camera_reload();
    }
    bool invert = env_on("BLUEWAKE_MOUSE_INVERT_Y", false);
    if (bw_menu_checkbox("Invert the mouse's up and down", &invert)) {
        set_env("BLUEWAKE_MOUSE_INVERT_Y", invert ? "1" : "0");
        bluewake_mouse_camera_reload();
    }
    if (ImGui::CollapsingHeader("Mouse buttons")) {
        BwMouseMap buttons;
        bw_mouse_map_parse(env("BLUEWAKE_MOUSE_BUTTONS").c_str(), &buttons);
        ImGui::TextWrapped("What each mouse button presses while the mouse is the camera. The first left click "
                           "only hands the mouse to the game.");
        if (bw_mouse_map_ui(&buttons)) {
            char text[64];
            bw_mouse_map_format(&buttons, text, sizeof text);
            set_env("BLUEWAKE_MOUSE_BUTTONS", text);
            bluewake_mouse_camera_reload();
        }
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    bool stick = env_on("BLUEWAKE_STICK_CAMERA", true);
    if (bw_menu_checkbox("Direct right-stick camera and aiming", &stick)) {
        set_env("BLUEWAKE_STICK_CAMERA", stick ? "1" : "0");
        bluewake_mouse_camera_reload();
    }
    ImGui::BeginDisabled(!stick);
    float speed = static_cast<float>(std::atof(env("BLUEWAKE_STICK_CAMERA_SPEED", "360").c_str()));
    if (speed <= 0.f)
        speed = 360.f;
    if (slider("Right-stick turn speed", &speed, 120.f, 720.f, "%.0f degrees a second")) {
        char text[16];
        std::snprintf(text, sizeof text, "%.0f", speed);
        set_env("BLUEWAKE_STICK_CAMERA_SPEED", text);
        bluewake_mouse_camera_reload();
    }
    float aim = static_cast<float>(std::atof(env("BLUEWAKE_STICK_AIM_SPEED", "180").c_str()));
    if (aim <= 0.f)
        aim = 180.f;
    if (slider("Right-stick aim speed (first person, items)", &aim, 60.f, 480.f,
               "%.0f degrees a second")) {
        char text[16];
        std::snprintf(text, sizeof text, "%.0f", aim);
        set_env("BLUEWAKE_STICK_AIM_SPEED", text);
        bluewake_mouse_camera_reload();
    }
    bool stick_zoom = env_on("BLUEWAKE_STICK_ZOOM", false);
    if (bw_menu_checkbox("Hold the right-stick click and move it up/down to zoom", &stick_zoom)) {
        set_env("BLUEWAKE_STICK_ZOOM", stick_zoom ? "1" : "0");
        bluewake_mouse_camera_reload();
    }
    ImGui::BeginDisabled(!stick_zoom);
    float zoom_speed = static_cast<float>(std::atof(env("BLUEWAKE_STICK_ZOOM_SPEED", "1.275").c_str()));
    if (!(zoom_speed > 0.f))
        zoom_speed = 1.275f;
    if (slider("Controller zoom speed", &zoom_speed, 0.25f, 2.f, "%.3fx")) {
        char text[16];
        std::snprintf(text, sizeof text, "%.3f", zoom_speed);
        set_env("BLUEWAKE_STICK_ZOOM_SPEED", text);
        bluewake_mouse_camera_reload();
    }
    ImGui::EndDisabled();
    bool invert_x = env_on("BLUEWAKE_STICK_CAMERA_INVERT_X", false);
    if (bw_menu_checkbox("Invert the right stick's left and right", &invert_x)) {
        set_env("BLUEWAKE_STICK_CAMERA_INVERT_X", invert_x ? "1" : "0");
        bluewake_mouse_camera_reload();
    }
    bool invert_y = env_on("BLUEWAKE_STICK_CAMERA_INVERT_Y", false);
    if (bw_menu_checkbox("Invert the right stick's up and down", &invert_y)) {
        set_env("BLUEWAKE_STICK_CAMERA_INVERT_Y", invert_y ? "1" : "0");
        bluewake_mouse_camera_reload();
    }
    ImGui::EndDisabled();
    bool aim_invert = env_on("BLUEWAKE_AIM_INVERT_Y", false);
    if (bw_menu_checkbox("Invert the left stick's up and down when aiming (first person, items)", &aim_invert)) {
        set_env("BLUEWAKE_AIM_INVERT_Y", aim_invert ? "1" : "0");
        bluewake_mouse_camera_reload();
    }
    bool gyro = env_on("BLUEWAKE_GYRO_AIM", false);
    if (bw_menu_checkbox("Gyro aiming: move the controller to aim (DualSense, DualShock 4, Switch Pro, Steam Deck)",
                        &gyro)) {
        set_env("BLUEWAKE_GYRO_AIM", gyro ? "1" : "0");
        bluewake_mouse_camera_reload();
    }
    ImGui::BeginDisabled(!gyro);
    float gyro_speed = static_cast<float>(std::atof(env("BLUEWAKE_GYRO_AIM_SPEED", "1").c_str()));
    if (!(gyro_speed > 0.f))
        gyro_speed = 1.f;
    if (slider("Gyro aim speed", &gyro_speed, 0.25f, 4.f, "%.2fx")) {
        char text[16];
        std::snprintf(text, sizeof text, "%.2f", gyro_speed);
        set_env("BLUEWAKE_GYRO_AIM_SPEED", text);
        bluewake_mouse_camera_reload();
    }
    ImGui::EndDisabled();
    ImGui::PushTextWrapPos();
    ImGui::TextDisabled(stick ? "Click the right stick for first person. When controller zoom is on, hold it and move the stick up/down. In the telescope and Picto Box, the left stick or D-pad zooms."
                              : "The game's right stick: its left and right follow Better Wind Waker's "
                                "\"Invert camera\" (Gameplay).");

    // Haptics (haptics.h): the game's vibration rendered from what it asked
    // for, with the triggers; or its own on-off motor; or none.
    ImGui::SeparatorText("Haptics");
    static const char* const kHaptics[] = {"Off", "Classic (the game's own on and off)",
                                           "Enhanced (shaped, with the triggers)"};
    const std::string mode = env("BLUEWAKE_HAPTICS", "enhanced");
    int haptics = mode.empty() || mode[0] == 'e' || mode[0] == '1' ? 2 : mode[0] == 'c' ? 1 : 0;
    if (bw_menu_combo("Controller vibration", &haptics, kHaptics, 3)) {
        set_env("BLUEWAKE_HAPTICS", haptics == 0 ? "off" : haptics == 1 ? "classic" : "enhanced");
        bluewake_haptics_reload();
    }
    ImGui::BeginDisabled(haptics != 2);
    // Its label above it, at full width, as the other sliders.
    float strength = static_cast<float>(std::clamp(std::atoi(env("BLUEWAKE_HAPTICS_STRENGTH", "80").c_str()), 0, 100));
    if (slider("Vibration strength", &strength, 0.f, 100.f, "%.0f%%")) {
        set_env("BLUEWAKE_HAPTICS_STRENGTH", std::to_string(static_cast<int>(std::lround(strength))));
        bluewake_haptics_reload();
        bluewake_haptics_preview();
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(haptics != 2);
    bool triggers = env_on("BLUEWAKE_HAPTICS_TRIGGERS", true);
    if (bw_menu_checkbox("Trigger feedback (Xbox impulse triggers, DualSense trigger vibration)", &triggers)) {
        set_env("BLUEWAKE_HAPTICS_TRIGGERS", triggers ? "1" : "0");
        bluewake_haptics_reload();
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled(haptics == 2   ? "Hits, falls, explosions and quakes as the game times them, shaped by their strength."
                        : haptics == 1 ? "The motor on and off, as a GameCube controller's."
                                       : "No vibration. (The game's own Vibration option turns it off too.)");

    // The keys as they are now (a remapped key shows its new name), in the same
    // table as on Windows; then the mouse and the controller, wrapped to fit.
    ImGui::SeparatorText("Keyboard");
    BwKeyMap keys;
    bw_key_map_parse(env("BLUEWAKE_KEY_MAP"), &keys);
    bw_keyboard_table(keys);
    ImGui::TextWrapped("Mouse: click the game, then move it to turn the camera and aim; left click is A and the "
                       "wheel zooms. Esc gives the mouse back, and Esc again opens this menu.");
    ImGui::SeparatorText("Controller");
    bw_controller_table();
    ImGui::PopTextWrapPos();
}

// The folder in the system's file manager.
void open_folder(const std::string& path) {
    std::string url = "file://";
    for (const unsigned char c : path) {
        if (std::isalnum(c) || std::strchr("/-._~", c) != nullptr) {
            url += static_cast<char>(c);
        } else {
            char escaped[4];
            std::snprintf(escaped, sizeof escaped, "%%%02X", c);
            url += escaped;
        }
    }
    if (!SDL_OpenURL(url.c_str()))
        std::fprintf(stderr, "[settings] could not open %s: %s\n", path.c_str(), SDL_GetError());
}

// Sound and files, as on Windows: the sound's engine (next launch), and where
// the saves, settings and logs are.
void sound_files_tab() {
    ImGui::SeparatorText("Sound");
    int exact = env("BLUEWAKE_DSP_MODE", "hle") == "lle" ? 1 : 0;
    bool picked = ImGui::RadioButton("Fast (Dolphin's high-level Zelda sound)", &exact, 0);
    picked |= ImGui::RadioButton("Exact (the sound chip's own program; slower)", &exact, 1);
    if (picked) {
        set_env("BLUEWAKE_DSP_MODE", exact ? "lle" : "hle");
        g_dirty = g_restart_pending = true;
    }
    restart_note();
    ImGui::Spacing();
    ImGui::SeparatorText("Your files");
    std::string folder = g_data_dir;
    if (folder.empty() && !g_path.empty())
        folder = g_path.substr(0, g_path.find_last_of('/') + 1);
    if (folder.empty()) {
        ImGui::TextDisabled("This session keeps no files.");
        return;
    }
    if (folder.back() != '/')
        folder += '/';
    ImGui::TextDisabled("Saves and session logs are kept apart from the build:");
    ImGui::TextUnformatted(folder.c_str());
    if (!g_path.empty())
        ImGui::TextDisabled("Settings: %s", g_path.c_str());
    if (ImGui::Button("Open that folder"))
        open_folder(folder);
    const std::string logs = folder + "logs";
    SDL_PathInfo info;
    if (SDL_GetPathInfo(logs.c_str(), &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
        ImGui::SameLine();
        if (ImGui::Button("Open the session logs"))
            open_folder(logs);
    }
}

void open_menu();
void close_menu();

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

// The climbing stamina wheel (climb.c), beside Link in the game's picture.
void draw_climb_wheel() {
    float fraction, x, y, aspect, alpha;
    bool exhausted;
    if (!bluewake_climb_hud(&fraction, &exhausted, &x, &y, &aspect, &alpha))
        return;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    float w = display.x, h = display.y, x0 = 0.f, y0 = 0.f;
    if (h <= 0.f || aspect <= 0.f)
        return;
    if (w / h > aspect) {
        w = h * aspect;
        x0 = (display.x - w) * 0.5f;
    } else {
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

float g_font_dpi = 1.f;
bool g_font_ready = false;
ImFont* g_menu_font = nullptr;

void load_mac_font() {
    SDL_Window* window = game_window();
    g_font_dpi = window != nullptr ? std::max(1.f, SDL_GetWindowDisplayScale(window)) : 1.f;
    auto* atlas = new ImFontAtlas(); // Process lifetime; never mutate Aurora's live atlas.
    atlas->Flags |= ImFontAtlasFlags_NoMouseCursors;
    ImFont* font = nullptr;
#if defined(__linux__)
    // A metric-compatible sans from common distro font packages; DejaVu first.
    static const char* const kLinuxFonts[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
    };
    for (const char* path : kLinuxFonts) {
        font = atlas->AddFontFromFileTTF(path, 17.f * g_font_dpi);
        if (font != nullptr) break;
    }
#else
    font = atlas->AddFontFromFileTTF("/System/Library/Fonts/SFNS.ttf", 17.f * g_font_dpi);
#endif
    if (font == nullptr) {
        ImFontConfig config;
        config.SizePixels = 17.f * g_font_dpi;
        font = atlas->AddFontDefault(&config);
    }
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    atlas->GetTexDataAsRGBA32(&pixels, &width, &height);
    if (pixels != nullptr && width > 0 && height > 0) {
        atlas->SetTexID(aurora_imgui_add_texture(width, height, pixels));
        atlas->ClearTexData();
        g_menu_font = font;
    } else {
        g_font_dpi = 1.f;
    }
}

// For the first 20 seconds, when Aurora fell back to OpenGL: it runs at a
// fraction of Vulkan's speed, and the only sign was the frame rate (#56).
void draw_renderer_notice(Uint64 now) {
#if !defined(__APPLE__)
    static const char* api = dol_aurora_backend_name();
    if (api == nullptr || std::strncmp(api, "OpenGL", 6) != 0 || now - g_installed_ms > 20000)
        return;
    ImGui::SetNextWindowPos(ImVec2(12.f, 12.f));
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("##renderer-notice", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::Text("Running on %s, which is much slower than Vulkan.", api);
    ImGui::TextUnformatted("Install the Vulkan loader (libvulkan1 or vulkan-loader) and your GPU's Vulkan driver.");
    ImGui::End();
#else
    (void)now;
#endif
}

void draw(void*) {
    static Uint64 checked;
    const Uint64 now = SDL_GetTicks();
    if (checked == 0 || now - checked >= 1000) { refresh_smooth_rate(); checked = now; }
    apply_controller_swaps();
    apply_key_map();
    if (!g_font_ready) {
        load_mac_font();
        g_font_ready = true;
        return;
    }
    test_hook();
    draw_climb_wheel();
    draw_renderer_notice(now);
    bluewake_ui::draw_status_line(now);
    draw_safe_mode_note(now);
    draw_shader_wait();
    if (bluewake_stage_select_take_question()) {
        open_menu();
        g_stage_select_question = g_open;
    }
    if (!g_open)
        return;
    ImGuiIO& io = ImGui::GetIO();
    if (!g_nav_set) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        g_nav_set = true;
    }
    // The controller, read here and handed to ImGui as keys (menu_shared.h).
    const BwMenuPad pad = bw_menu_controller(env("BLUEWAKE_BUTTON_MAP"), env_on("BLUEWAKE_PAD_SWAP_AB", false));
    const ImVec2 display = io.DisplaySize;
    if (g_stage_select_question) {
        if (pad.back) {  // B cancels the question, as Esc does
            io.AddKeyEvent(ImGuiKey_Escape, true);
            io.AddKeyEvent(ImGuiKey_Escape, false);
        }
        ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.94f);
        bluewake_ui::begin_theme();
        if (g_menu_font != nullptr) ImGui::PushFont(g_menu_font);
        int answer = 0;
        if (ImGui::Begin("Developers' stage select", nullptr,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::SetWindowFontScale(1.f / g_font_dpi);
            answer = bw_stage_select_question();
        }
        ImGui::End();
        if (g_menu_font != nullptr) ImGui::PopFont();
        bluewake_ui::end_theme();
        if (answer != 0)
            close_menu();
        return;
    }
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(display.x - 24.f, 860.f), display.y * 0.78f), ImGuiCond_Appearing);
    // Resizing stays between a size that still holds the tabs, hint and buttons
    // and the edges of the game's window, less a margin, from where the menu is.
    const ImVec2 largest(std::max(1.f, display.x - 24.f), std::max(1.f, display.y - 24.f));
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(std::min(420.f, largest.x), std::min(300.f, largest.y)), largest, [](ImGuiSizeCallbackData* data) {
            // Growing stops at the edge it is dragged towards: the room right of the
            // left edge (dragging the right one) or left of the right edge (dragging
            // the left one), whichever is larger, as the callback isn't told which;
            // a menu that goes past an edge is moved back in before Begin below. A
            // menu already past it (moved there) isn't shrunk.
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
    bluewake_ui::begin_theme();
    if (g_menu_font != nullptr) ImGui::PushFont(g_menu_font);
    bool open = true;
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
    // A click on the game or outside the window leaves no window focused, and
    // then the controller has nothing to move through: give the menu focus back.
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
        ImGui::SetNextWindowFocus();
    if (ImGui::Begin("BlueWake settings", &open,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::SetWindowFontScale(1.f / g_font_dpi);
        // The bumpers switch tabs. Each tab's body scrolls on its own but is
        // flattened for navigation, so the D-pad moves from the tabs through the
        // options to the buttons below without A to enter it and B to leave it.
        struct Tab { const char* label; const char* body; void (*draw)(); };
        static const Tab kTabs[] = {{"Display", "##display-body", display_tab},
                                    {"Gameplay", "##gameplay-body", gameplay_tab},
                                    {"Mods", "##mods-body", mods_tab},
                                    {"Controls", "##controls-body", controls_tab},
                                    {"Sound and files", "##sound-files-body", sound_files_tab}};
        constexpr int kTabCount = static_cast<int>(sizeof kTabs / sizeof kTabs[0]);
        static int shown = 0;
        int request = -1;
        if (pad.tab_step != 0)
            request = (shown + kTabCount + pad.tab_step) % kTabCount;
        // The body takes what the footer leaves: the hint, the next-launch note
        // when there is one, and the buttons on one row or, if narrow, two.
        const float footer = bw_menu_footer_height(g_restart_pending);
        if (ImGui::BeginTabBar("##tabs")) {
            // The window has no title bar, so the close button for the mouse sits at the end of the tabs.
            if (ImGui::TabItemButton("x", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                open = false;
            for (int i = 0; i < kTabCount; ++i) {
                if (!ImGui::BeginTabItem(kTabs[i].label, nullptr, i == request ? ImGuiTabItemFlags_SetSelected : 0))
                    continue;
                shown = i;
                ImGui::BeginChild(kTabs[i].body, ImVec2(0, -footer), ImGuiChildFlags_NavFlattened);
                ImGui::PushTextWrapPos(0.f);
                kTabs[i].draw();
                ImGui::PopTextWrapPos();
                bw_menu_stick_scroll(pad.scroll);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        bw_menu_footer_note(g_restart_pending);
        if (g_restart_pending) {
#if defined(__linux__)
            // Linux's launcher (linux_entry.c) starts the game again after a
            // clean exit; the Mac app has no relaunch yet.
            ImGui::SameLine();
            if (ImGui::Button("Restart now")) {
                g_dirty = true;
                save();
                g_restart_requested = true;
                close_menu();
                SDL_Event quit{};
                quit.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit);
            }
#endif
        }
        switch (bw_menu_footer_buttons(g_path.c_str())) {
        case BW_MENU_RESUME:
            open = false;
            break;
        // Save states (debugging): taken or put back at the game's next clean
        // point once the menu has closed (main.c's host_state_*).
        case BW_MENU_SAVE_STATE:
            bluewake_save_state_hotkey(false);
            open = false;
            break;
        case BW_MENU_LOAD_STATE:
            bluewake_save_state_hotkey(true);
            open = false;
            break;
        case BW_MENU_QUIT: {
            close_menu();
            SDL_Event quit{};
            quit.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quit);
            break;
        }
        case BW_MENU_NONE:
            break;
        }
        // The game's B ends an edit, closes a dropdown, or closes the menu (menu_shared.h).
        if (bw_menu_back(pad.back))
            open = false;
    }
    ImGui::End();
    if (g_menu_font != nullptr) ImGui::PopFont();
    bluewake_ui::end_theme();
    if (!open)
        close_menu();
}

// Safe mode (BLUEWAKE_SAFE_MODE: --safe-mode, or a launch that never got to a
// clean exit, from linux_entry.c), as on Windows: the settings file is copied
// aside, then this session and the file get the safe choices: Fast sound,
// 4:3, in a window, no HD look, mods, options or texture pack, and the gameplay extras
// off. If the copy fails the file is left alone and only this session is safe.
void apply_safe_mode() {
    const char* asked = std::getenv("BLUEWAKE_SAFE_MODE");
    if (asked == nullptr || asked[0] == '\0' || asked[0] == '0')
        return;
    g_safe_mode = true;
    const bool backed_up = !g_path.empty() && bw_launch_backup(g_path.c_str());
    g_safe_backed_up = backed_up;
    if (!backed_up)
        std::fprintf(stderr, g_path.empty() ? "[safe-mode] no settings file; only this session is reset\n"
                                            : "[safe-mode] settings backup failed; only this session is reset\n");
    static const char* const kSafe[][2] = {
        {"BLUEWAKE_DSP_MODE", "hle"},        {"BLUEWAKE_MODS", "none"},
        {"BLUEWAKE_OPTIONS", "none"},        {"BLUEWAKE_ASPECT", "4:3"},
        {"DOL_AURORA_ASPECT_FIT", ""},       {"DOL_AURORA_TEXTURE_PACK", ""},
        {"DOL_AURORA_FULLSCREEN", "0"},      {"BLUEWAKE_JUMP_BUTTON", "0"},
        {"BLUEWAKE_SPRINT_SPEED", "1"},      {"BLUEWAKE_FAST_FORWARD", "0"},
        {"BLUEWAKE_FADE_FRAMES", "0"},       {"BLUEWAKE_QUICK_DOORS", "0"},
        {"BLUEWAKE_STAGE_SELECT_TOGGLE", "0"},
        {"BLUEWAKE_HD_LOOK", "0"},
        // Clear overrides so the HD switch can enable every effect again later.
        {"BLUEWAKE_HD_DISTANT_ISLANDS", ""}, {"BLUEWAKE_HD_BLOOM", ""},
        {"BLUEWAKE_HD_FAR_BLUR", ""},        {"BLUEWAKE_HD_AO", ""},
        {"BLUEWAKE_HD_CLOUD_SHADOWS", ""},   {"BLUEWAKE_HD_SUN_SHADOWS", ""},
        {"BLUEWAKE_HD_EDGE_LIGHT", ""},      {"BLUEWAKE_HD_TORCH_LIGHT", ""},
    };
    for (const auto& pair : kSafe) {
        setenv(pair[0], pair[1], 1);
        // Per-effect overrides are otherwise kept verbatim as unmanaged keys.
        if (backed_up)
            g_other.erase(pair[0]);
    }
    std::fprintf(stderr, "[safe-mode] Fast sound, 4:3, a window, no HD look, mods or options%s\n",
                 backed_up ? "; the earlier settings were copied beside settings.ini" : "");
    if (backed_up) {
        g_dirty = true;
        save();
    }
}

} // namespace

extern "C" void bluewake_settings_load(void) {
    const char* chosen = std::getenv("BLUEWAKE_SETTINGS");
    // No settings file (BLUEWAKE_SETTINGS=none, or no home folder): safe mode
    // still applies, to this session only.
    if (chosen != nullptr && std::strcmp(chosen, "none") == 0) {
        apply_safe_mode();
        return;
    }
    g_path = chosen != nullptr && chosen[0] != '\0' ? chosen : default_path();
    if (g_path.empty()) {
        apply_safe_mode();
        return;
    }
    // Jump and sprint start off, as on the iPad and Windows (#71); a saved choice
    // or an explicit environment value wins.
    setenv("BLUEWAKE_JUMP_BUTTON", "0", 0);
    setenv("BLUEWAKE_SPRINT_SPEED", "1", 0);
    FILE* file = std::fopen(g_path.c_str(), "r");
    // Read legacy preferences if needed, but never rename or overwrite the old file.
    if (file == nullptr && chosen == nullptr) {
        const char* user_home = std::getenv("HOME");
        if (user_home != nullptr) {
            const std::string legacy = std::string(user_home) +
                "/Library/Application Support/Wind Waker Recomp/settings.ini";
            file = std::fopen(legacy.c_str(), "r");
        }
    }
    if (file == nullptr) {
        apply_safe_mode();
        return;
    }
    char line[2048];
    int count = 0;
    while (std::fgets(line, sizeof line, file) != nullptr) {
        std::string text = trim(line);
        if (text.empty() || text[0] == '#')
            continue;
        const auto equals = text.find('=');
        if (equals == std::string::npos || equals == 0)
            continue;
        const std::string key = trim(text.substr(0, equals));
        const std::string value = trim(text.substr(equals + 1));
        setenv(key.c_str(), value.c_str(), 1);
        if (!managed(key))
            g_other[key] = value;
        ++count;
    }
    std::fclose(file);
    std::fprintf(stderr, "[settings] %d settings from %s\n", count, g_path.c_str());
    apply_safe_mode();
}

extern "C" bool bluewake_settings_restart_requested(void) { return g_restart_requested; }

extern "C" void bluewake_settings_set_data_dir(const char* dir) { g_data_dir = dir != nullptr ? dir : ""; }

extern "C" void bluewake_settings_menu_install(void) {
    g_installed_ms = SDL_GetTicks();
    if (const char* test = std::getenv("BLUEWAKE_SETTINGS_TEST_OPEN"))
        if (std::sscanf(test, "%lf:%lf", &g_test_open_at, &g_test_open_for) != 2)
            g_test_open_at = -1.0;
    dol_aurora_set_overlay(draw, nullptr);
    aurora_set_pause_on_focus_lost(env_on("BLUEWAKE_PAUSE_UNFOCUSED", false));
    g_shader_wait = env_on("BLUEWAKE_SHADERS_FIRST", false);
    dol_aurora_set_hold(hold, nullptr);
    dol_aurora_set_hold_redraw(true);
    std::fprintf(stderr, "[settings] Esc (with the mouse free), F1 or a controller's Back opens the options\n");
}

extern "C" bool bluewake_settings_menu_event(const void* sdl_event) {
    const SDL_Event* event = static_cast<const SDL_Event*>(sdl_event);
    switch (event->type) {
    case SDL_EVENT_KEY_DOWN:
        if (event->key.repeat)
            break;
        if (event->key.scancode == SDL_SCANCODE_F1) {
            g_open ? close_menu() : open_menu();
            return true;
        }
        // F2: the Wind Waker HD look on or off, at once, and kept (#108).
        if (event->key.scancode == SDL_SCANCODE_F2) {
            const bool on = !env_on("BLUEWAKE_HD_LOOK", false);
            set_env("BLUEWAKE_HD_LOOK", on ? "1" : "0");
            bluewake_hd_look_reload();
            save();
            bluewake_status_post(on ? "HD look on" : "HD look off");
            std::fprintf(stderr, "[settings] Wind Waker HD look %s (F2)\n", on ? "on" : "off");
            return true;
        }
        if (event->key.scancode == SDL_SCANCODE_F9) {
            const bool on = !env_on("DOL_AURORA_SHOW_FPS", false);
            set_show_fps(on);
            save();
            bluewake_status_post(on ? "Frame rate on" : "Frame rate off");
            return true;
        }
        if (event->key.scancode == SDL_SCANCODE_F10) {
            // On at the rate chosen in the menu (60 if none), or off.
            const bool on = !env_on("DOL_AURORA_FRAME_INTERP", false);
            const std::string steps = env("DOL_AURORA_FRAME_INTERP_STEPS", "1");
            set_smooth_motion(on ? steps.c_str() : nullptr);
            save();
            bluewake_status_post(on ? "Smooth Motion on" : "Smooth Motion off");
            std::fprintf(stderr, "[settings] Smooth Motion %s (F10)\n", on ? "on" : "off");
            return true;
        }
        if (event->key.scancode == SDL_SCANCODE_F11 ||
            (event->key.scancode == SDL_SCANCODE_RETURN && (event->key.mod & SDL_KMOD_ALT) != 0)) {
            // Return is the keyboard's START, and the pad reads the keyboard
            // itself: Alt+Enter also paused or unpaused the game. Held from
            // the game until it is let go, as the press that closes the menu
            // is (Windows's keyboard hook keeps Alt+Return from SDL instead).
            if (event->key.scancode == SDL_SCANCODE_RETURN)
                bluewake_pad_hold_until_released();
            const bool on = !is_fullscreen();
            set_fullscreen(on);
            bluewake_status_post(on ? "Fullscreen on" : "Fullscreen off");
            return true;
        }
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
            if (g_open) {
                close_menu();
                return true;
            }
            // Esc with the mouse as the camera gives the mouse back first.
            if (!bluewake_mouse_camera_captured()) {
                open_menu();
                return true;
            }
            return false;
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
            g_open ? close_menu() : open_menu();
            return true;
        }
        break;
    default:
        break;
    }
    // While it is open the menu (ImGui) has the input to itself.
    return g_open;
}
