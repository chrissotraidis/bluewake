// BlueWake for Windows: the settings menu's plumbing. The menu itself, the
// settings and their file are runtime/host/src/desktop_menu.cpp, shared with
// Mac and Linux; this is what only Windows does around it:
//
// - the settings file in %APPDATA%\BlueWake (or the portable app's folder), a
//   file from before the menus were shared turned into the shared format, and
//   the window's size and place kept in it;
// - the keys (F1 or Esc, F11 and Alt+Enter, F10, F9, F2 and F6) from the game
//   thread's keyboard hook, before SDL sees them (win_entry.c);
// - the game keeps running under the menu, its input kept from it;
// - Restart now, which starts BlueWake again with the environment it began
//   with (CreateProcessW), and the launch marker, cleared once 600 frames have
//   gone by healthy;
// - the UI's scale and font, the first seconds' hint, and the frame rate line
//   in the session log.
#include "win_settings.h"
#include "restart_request.h"
#include "launch_marker.h"

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
#include "desktop_menu.h"
#include "desktop_theme.h"
#include "gxruntime/aurora_backend.h"
#include "save_state.h"

#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
// runtime/host/src/mouse_camera.h, declared here with plain types: the header
// brings in the guest CPU's, which this file has no use for.
void bluewake_mouse_camera_block(bool blocked);
bool bluewake_mouse_camera_captured(void);
}

// Aurora's frame counters (lib/gfx/common.hpp, linked in statically), for the
// session log: every present, and the game's own frames.
namespace aurora::gfx {
float calculate_fps() noexcept;
float calculate_game_fps() noexcept;
}  // namespace aurora::gfx

namespace {

// The window's size and place (settings_table.h's Windows-only rows).
constexpr const char* kBwWindowKey = "DOL_AURORA_WINDOW";
constexpr const char* kBwWindowPositionKey = "BLUEWAKE_WINDOW_POSITION";

std::string g_data_dir;
std::vector<wchar_t> g_environment;  // as BlueWake was started, for Restart
RestartRequest g_restart;
Uint64 g_first_frame_at;
float g_font_scale = 1.0f;  // the scale the UI font was drawn at (see load_font)
// The menu's own font: the UI font drawn kTextLargest times larger (desktop_theme.h),
// so the menu's largest text size is still sharp; nullptr while it is ImGui's.
ImFont* g_menu_font = nullptr;
float g_menu_font_larger = 1.0f;

// The keys come on the game thread; what they do is done on the main thread,
// in the frame, where the menu and its settings live.
enum Hotkey : unsigned { kMenu = 1, kFullscreen = 2, kSmooth = 4, kFps = 8, kHdLook = 16 };
std::atomic<unsigned> g_hotkeys{0};

// Load\Textures\GZLE01, where a pack goes when none is chosen.
std::string default_pack_folder() { return g_data_dir + "Load\\Textures\\GZLE01"; }

double aspect_ratio(const std::string& aspect) {
    return aspect == "16:9" ? 16.0 / 9.0 : aspect == "16:10" ? 1.6 : aspect == "21:9" ? 21.0 / 9.0 : 4.0 / 3.0;
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

// Whether a window whose client area starts at (x, y) has its title bar and
// its top left on a connected monitor. Asked at launch, before the window
// exists, so Win32's rather than SDL's; the manifest makes BlueWake per-monitor
// DPI aware, so these are the pixels SDL saved the position in.
bool on_a_monitor(int x, int y) {
    const POINT title{x + 60, y - 16};
    const POINT corner{x + 60, y + 60};
    return MonitorFromPoint(title, MONITOR_DEFAULTTONULL) != nullptr &&
           MonitorFromPoint(corner, MONITOR_DEFAULTTONULL) != nullptr;
}

// Remember the window's place and size as the player leaves them
// (DOL_AURORA_WINDOW and BLUEWAKE_WINDOW_POSITION, settings_table.h's
// Windows-only rows).
void track_window(SDL_Window* w) {
    const SDL_WindowFlags flags = SDL_GetWindowFlags(w);
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED | SDL_WINDOW_MAXIMIZED))
        return;
    int x = 0, y = 0, width = 0, height = 0;
    if (!SDL_GetWindowPosition(w, &x, &y) || !SDL_GetWindowSize(w, &width, &height))
        return;
    static bool first = true;
    static int last_x, last_y, last_w, last_h;
    const bool moved = x != last_x || y != last_y || width != last_w || height != last_h;
    last_x = x;
    last_y = y;
    last_w = width;
    last_h = height;
    // Where it opened is what the file already holds.
    if (first || !moved) {
        first = false;
        return;
    }
    bw_desktop::set_env(kBwWindowKey, std::to_string(width) + "x" + std::to_string(height));
    bw_desktop::set_env(kBwWindowPositionKey, std::to_string(x) + "," + std::to_string(y));
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
    ImFont* menu = nullptr;
    char fonts[MAX_PATH];
    const UINT length = GetWindowsDirectoryA(fonts, MAX_PATH);
    if (length > 0 && length < MAX_PATH - 32) {
        const std::string path = std::string(fonts) + "\\Fonts\\segoeui.ttf";
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
            font = atlas->AddFontFromFileTTF(path.c_str(), std::round(15.0f * scale), nullptr, ranges);
        if (font != nullptr)
            menu = atlas->AddFontFromFileTTF(path.c_str(), std::round(15.0f * scale * bluewake_ui::kTextLargest),
                                             nullptr, ranges);
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
    g_menu_font = menu;
    g_menu_font_larger = menu != nullptr ? bluewake_ui::kTextLargest : 1.0f;
    ImGui::GetStyle().ScaleAllSizes(scale);
    g_font_scale = scale;
    std::fprintf(stderr, "[windows] UI scale %.2f (font atlas %dx%d)\n", scale, width, height);
}

// For the first few seconds, where the settings and fullscreen are.
void draw_hint(SDL_Window* w) {
    const Uint64 shown = SDL_GetTicks() - g_first_frame_at;
    if (shown > 7000 || bw_desktop::menu_open())
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

// The frame rate in the session log, a line a second beside the host's [perf]
// line: frames shown (60 with Smooth Motion at full speed) and the game's own
// (30 at full speed). With it, where the game thread waited in that second
// (GXRuntime's counters): for the GX worker at the game's draw-done barriers,
// and in the present, of which end_frame is Aurora's submission.
void log_frame_rate(Uint64 now) {
    static Uint64 logged;
    static DolAuroraFrameTiming before;
    if (now - logged < 1000)
        return;
    DolAuroraFrameTiming timing{};
    dol_aurora_frame_timing(&timing);
    if (logged != 0)
        std::fprintf(stderr, "[fps] shown=%.1f game=%.1f drain_ms=%.0f present_ms=%.0f end_frame_ms=%.0f\n",
                     aurora::gfx::calculate_fps(), aurora::gfx::calculate_game_fps(),
                     (timing.drain_us - before.drain_us) / 1000.0, (timing.present_us - before.present_us) / 1000.0,
                     (timing.end_frame_us - before.end_frame_us) / 1000.0);
    logged = now;
    before = timing;
}

void save_state_key() { bluewake_save_state_hotkey(false); }

}  // namespace

// What the shared menu asks of Windows.
namespace bw_platform {

bool ready(SDL_Window* window) {
    if (g_first_frame_at != 0)
        return true;
    g_first_frame_at = SDL_GetTicks();
    load_font(window);
    return false;  // the font is ImGui's default from the next frame
}

Look look(SDL_Window* window) {
    Look look;
    look.scale = ui_scale(window);
    look.font = g_menu_font;
    look.font_scale = look.scale / (g_font_scale * g_menu_font_larger);
    return look;
}

void frame(SDL_Window* window, unsigned long long now_ms) {
    const unsigned keys = g_hotkeys.exchange(0);
    if (keys & kMenu)
        bw_desktop::menu_open() ? bw_desktop::close_menu() : bw_desktop::open_menu();
    if (keys & kFullscreen)
        bw_desktop::toggle_fullscreen();
    if (keys & kSmooth)
        bw_desktop::toggle_smooth_motion();
    if (keys & kFps)
        bw_desktop::toggle_fps();
    if (keys & kHdLook)
        bw_desktop::toggle_hd_look();
    track_window(window);
    log_frame_rate(now_ms);
    static unsigned healthy_frames;
    if (!bw_desktop::menu_open() && ++healthy_frames == 600u &&
        !bw_launch_clear((g_data_dir + "launch.pending").c_str()))
        std::fprintf(stderr, "[safe-mode] could not clear launch marker\n");
    draw_hint(window);
}

// The game's keys, pad and mouse are kept from it while the menu is open, so
// the menu's clicks and keys do not move Link (the game may run on under it:
// BLUEWAKE_MENU_PAUSES).
void menu_opened(bool open) {
    PADBlockInput(open);
    PADSetKeyboardActive(0, open ? FALSE : TRUE);
    bluewake_mouse_camera_block(open);
}

std::string default_pack_folder() { return ::default_pack_folder(); }
std::string textures_folder() { return g_data_dir + "Load\\Textures"; }

void open_folder(const std::string& path) {
    CreateDirectoryA(path.c_str(), nullptr);
    wchar_t wide[MAX_PATH * 2];
    if (MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide, MAX_PATH * 2) > 0)
        ShellExecuteW(nullptr, L"open", wide, nullptr, nullptr, SW_SHOWNORMAL);
}

void reset_window() {
    SDL_Window* w = bw_desktop::game_window();
    if (w == nullptr)
        return;
    SDL_SetWindowFullscreen(w, false);
    int width = 0, height = 0;
    default_window(aspect_ratio(bw_desktop::env("BLUEWAKE_ASPECT", "4:3")), &width, &height);
    SDL_SetWindowSize(w, width, height);
    SDL_SetWindowPosition(w, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

bool can_restart() { return true; }

// The settings are saved (the menu closed first); the game quits, and
// bw_settings_relaunch starts it again once the host has shut down.
void restart() {
    bw_desktop::save();
    const bool ok = g_restart.request(!bw_desktop::unsaved(), [] {
        SDL_Event quit{};
        quit.type = SDL_EVENT_QUIT;
        return SDL_PushEvent(&quit);
    });
    if (!ok)
        std::fprintf(stderr, "[windows] restart refused: settings save or quit request failed\n");
}


}  // namespace bw_platform

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
    bw_desktop::set_data_dir(g_data_dir);
    bw_desktop::load(g_data_dir + "settings.ini");
    // Safe mode: --safe-mode, or a launch that never got to a clean exit.
    if (bw_desktop::env_on("BLUEWAKE_SAFE_MODE", false) || bw_launch_pending((g_data_dir + "launch.pending").c_str()))
        bw_desktop::apply_safe_mode();
    // Every default is put in the environment, as Windows always has, so the
    // file the menu writes is whole (settings_table.h's fill).
    bw_desktop::fill_defaults();
}

extern "C" void bw_settings_apply_launch(void) {
    using bw_desktop::env_default;
    // BLUEWAKE_STAGE_SELECT=1 on the command line arms it for this session (the
    // stage select takes either key, stage_select.h); the file keeps its toggle.
    if (const char* armed = std::getenv("BLUEWAKE_STAGE_SELECT"); armed != nullptr && armed[0] != '\0')
        bw_desktop::launch_choice("BLUEWAKE_STAGE_SELECT_TOGGLE", armed[0] == '1' ? "1" : "0");
    // The stage select's English names, which the builder installs beside BlueWake.exe.
    {
        char exe[MAX_PATH];
        const DWORD n = GetModuleFileNameA(nullptr, exe, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            std::string names(exe, n);
            names = names.substr(0, names.find_last_of("\\/") + 1) + "stage_select";
            const DWORD attributes = GetFileAttributesA(names.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
                env_default("BLUEWAKE_STAGE_SELECT_NAMES", names);
        }
    }
    CreateDirectoryA((g_data_dir + "Load").c_str(), nullptr);
    CreateDirectoryA((g_data_dir + "Load\\Textures").c_str(), nullptr);
    CreateDirectoryA(default_pack_folder().c_str(), nullptr);
    // The window's size as it was left, or one that suits this screen for this
    // session (the file keeps what it had).
    int width = 0, height = 0;
    if (std::sscanf(bw_desktop::env(kBwWindowKey).c_str(), "%dx%d", &width, &height) != 2 || width < 320 ||
        height < 240) {
        default_window(aspect_ratio(bw_desktop::env("BLUEWAKE_ASPECT", "4:3")), &width, &height);
        bw_desktop::launch_choice(kBwWindowKey, std::to_string(width) + "x" + std::to_string(height));
    }
    // Where the window was last time, for the host to create it there
    // (runtime/host/src/main.c): Aurora shows the window before its first
    // frame, and nothing moves it after. Otherwise it opens centred: the first
    // launch, a spot no longer on a monitor, or a fullscreen start, which keeps
    // the display it always had. Aurora takes no negative position, so a spot
    // left of or above the main display opens centred too. The file keeps the
    // spot either way.
    int x = 0, y = 0;
    if (std::sscanf(bw_desktop::env(kBwWindowPositionKey).c_str(), "%d,%d", &x, &y) == 2) {
        const bool fullscreen = bw_desktop::env_on("DOL_AURORA_FULLSCREEN", false);
        if (!fullscreen && !on_a_monitor(x, y))
            std::fprintf(stderr, "[windows] window centred: %d,%d is no longer on a monitor\n", x, y);
        else if (!fullscreen && (x < 0 || y < 0))
            std::fprintf(stderr, "[windows] window centred: Aurora cannot open it at %d,%d (negative)\n", x, y);
        if (fullscreen || !on_a_monitor(x, y) || x < 0 || y < 0)
            bw_desktop::launch_choice(kBwWindowPositionKey, "");
    }
}

extern "C" void bw_settings_install(void) { bw_desktop::install(); }

extern "C" int bw_settings_key(unsigned virtual_key, int alt) {
    switch (virtual_key) {
    case VK_F1:
        g_hotkeys |= kMenu;
        return 1;
    case VK_ESCAPE:
        // Esc first gives the mouse back (the mouse camera sees it); then it
        // opens and closes the menu.
        if (!bw_desktop::menu_open() && bluewake_mouse_camera_captured())
            return 0;
        g_hotkeys |= kMenu;
        return 1;
    case VK_F11:
        g_hotkeys |= kFullscreen;
        return 1;
    case VK_RETURN:
        if (!alt)
            return 0;
        g_hotkeys |= kFullscreen;
        return 1;
    case VK_F10:
        g_hotkeys |= kSmooth;
        return 1;
    case VK_F9:
        g_hotkeys |= kFps;
        return 1;
    case VK_F2:
        // The Wind Waker HD look on or off, at once, and kept (#108).
        g_hotkeys |= kHdLook;
        return 1;
    case VK_F6:
        // A second save key, unlisted: Wind Waker Recomp's, which players
        // learned. F5 (save) and F8 (load) are the shared keys (mouse_camera.c),
        // the same on every build (#320).
        save_state_key();
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
        bw_desktop::menu_open() ? bw_desktop::close_menu() : bw_desktop::open_menu();
        return true;
    }
    return bw_desktop::menu_open();
}
