// The settings menu's plumbing on Mac and Linux. The menu itself, the settings
// and their file are desktop_menu.cpp, shared with Windows; this is where the
// file lives, the font the menu draws in, the keys that reach it (SDL's
// events), Linux's OpenGL notice, and Linux's Restart now.
#include "settings_menu.h"
#include "desktop_menu.h"
#include "desktop_theme.h"

// The host's modules are C.
extern "C" {
#include "mouse_camera.h"
}

#include "gxruntime/aurora_backend.h"

#include <SDL3/SDL.h>
#include <aurora/imgui.h>
#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

namespace {

bool g_restart_requested = false;  // Restart now: linux_entry.c starts the game again
Uint64 g_started_ms = 0;

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

// The folder scripts/import_wwhd_textures.py puts its pack in.
std::string importer_pack_folder() {
    // Follow the app's data/settings location, including Linux portable mode
    // and isolated sessions. The normal Mac/Linux locations stay the same.
    std::filesystem::path base = bw_desktop::data_dir();
    if (base.empty() && !bw_desktop::settings_path().empty())
        base = std::filesystem::path(bw_desktop::settings_path()).parent_path();
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

// The menu's font, drawn at the display's scale so it is sharp on a Retina or
// HiDPI screen, in an atlas of its own (Aurora's live atlas is never changed).
float g_font_dpi = 1.f;
ImFont* g_menu_font = nullptr;

// The menu's font, drawn at the display's scale times the largest text size
// (desktop_theme.h) and shown smaller, so every text size is sharp.
void load_font(SDL_Window* window) {
    g_font_dpi = window != nullptr ? std::max(1.f, SDL_GetWindowDisplayScale(window)) : 1.f;
    const float font_px = 17.f * g_font_dpi * bluewake_ui::kTextLargest;
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
        font = atlas->AddFontFromFileTTF(path, font_px);
        if (font != nullptr) break;
    }
#else
    font = atlas->AddFontFromFileTTF("/System/Library/Fonts/SFNS.ttf", font_px);
#endif
    if (font == nullptr) {
        ImFontConfig config;
        config.SizePixels = font_px;
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
    if (api == nullptr || std::strncmp(api, "OpenGL", 6) != 0 || now - g_started_ms > 20000)
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

void toggle_menu() { bw_desktop::menu_open() ? bw_desktop::close_menu() : bw_desktop::open_menu(); }

}  // namespace

// What the shared menu asks of Mac and Linux.
namespace bw_platform {

bool ready(SDL_Window* window) {
    static bool loaded = false;
    if (loaded)
        return true;
    load_font(window);
    loaded = true;
    return false;  // the font is drawn from the next frame
}

Look look(SDL_Window*) {
    Look look;
    look.font_scale = g_menu_font != nullptr ? 1.f / (g_font_dpi * bluewake_ui::kTextLargest) : 1.f;
    look.font = g_menu_font;
    return look;
}

void frame(SDL_Window*, unsigned long long now_ms) { draw_renderer_notice(now_ms); }

// The mouse is handed back while the menu is open.
void menu_opened(bool open) {
    if (open)
        bluewake_mouse_camera_release();
}

std::string default_pack_folder() {
    std::error_code error;
    const std::string folder = importer_pack_folder();
    return !folder.empty() && std::filesystem::is_directory(folder, error) ? folder : "";
}

std::string textures_folder() { return std::filesystem::path(importer_pack_folder()).parent_path().string(); }

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

// Out of fullscreen, at the size the window starts at (DOL_AURORA_WINDOW, or
// 720 tall in the picture's shape), in the middle of the screen.
void reset_window() {
    SDL_Window* window = bw_desktop::game_window();
    if (window == nullptr)
        return;
    SDL_SetWindowFullscreen(window, false);
    int width = 0, height = 0;
    const std::string chosen = bw_desktop::env("DOL_AURORA_WINDOW");
    if (std::sscanf(chosen.c_str(), "%dx%d", &width, &height) != 2 || width < 320 || height < 240) {
        const float ratio = std::strtof(bw_desktop::env("DOL_AURORA_ASPECT_RATIO", "0").c_str(), nullptr);
        height = 720;
        width = static_cast<int>(height * (ratio > 1.f && ratio < 4.f ? ratio : 4.f / 3.f) + 0.5f);
    }
    SDL_SetWindowSize(window, width, height);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

// Linux's launcher (linux_entry.c) starts the game again after a clean exit;
// the Mac app has no relaunch yet.
bool can_restart() {
#if defined(__linux__)
    return true;
#else
    return false;
#endif
}

void restart() {
    g_restart_requested = true;
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
}


}  // namespace bw_platform

extern "C" void bluewake_settings_load(void) {
    // BLUEWAKE_SETTINGS=path chooses the file, none reads and writes none.
    const char* chosen = std::getenv("BLUEWAKE_SETTINGS");
    const bool none = chosen != nullptr && std::strcmp(chosen, "none") == 0;
    const std::string path = none ? "" : chosen != nullptr && chosen[0] != '\0' ? chosen : default_path();
    // Wind Waker Recomp's settings are read if BlueWake has none of its own yet
    // (they are never renamed or written over).
    std::string legacy;
    const char* home = std::getenv("HOME");
    if (chosen == nullptr && home != nullptr)
        legacy = std::string(home) + "/Library/Application Support/Wind Waker Recomp/settings.ini";
    bw_desktop::load(path, legacy);
    // Safe mode: --safe-mode, or a launch that never got to a clean exit
    // (linux_entry.c sets BLUEWAKE_SAFE_MODE).
    if (bw_desktop::env_on("BLUEWAKE_SAFE_MODE", false))
        bw_desktop::apply_safe_mode();
    // The defaults the game must not take its own for (settings_table.h's
    // fill): Fast sound, and Jump & Sprint off (#71).
    bw_desktop::fill_defaults();
}

extern "C" bool bluewake_settings_restart_requested(void) { return g_restart_requested; }

extern "C" void bluewake_settings_set_data_dir(const char* dir) { bw_desktop::set_data_dir(dir != nullptr ? dir : ""); }

extern "C" void bluewake_settings_menu_install(void) {
    g_started_ms = SDL_GetTicks();
    bw_desktop::install();
}

extern "C" bool bluewake_settings_menu_event(const void* sdl_event) {
    const SDL_Event* event = static_cast<const SDL_Event*>(sdl_event);
    switch (event->type) {
    case SDL_EVENT_KEY_DOWN:
        if (event->key.repeat)
            break;
        switch (event->key.scancode) {
        case SDL_SCANCODE_F1:
            toggle_menu();
            return true;
        case SDL_SCANCODE_F2:
            bw_desktop::toggle_hd_look();
            return true;
        case SDL_SCANCODE_F9:
            bw_desktop::toggle_fps();
            return true;
        case SDL_SCANCODE_F10:
            bw_desktop::toggle_smooth_motion();
            return true;
        case SDL_SCANCODE_RETURN:
            if ((event->key.mod & SDL_KMOD_ALT) == 0)
                break;
            // Return is the keyboard's START, and the pad reads the keyboard
            // itself: Alt+Enter also paused or unpaused the game. Held from the
            // game until it is let go, as the press that closes the menu is
            // (Windows's keyboard hook keeps Alt+Return from SDL instead).
            bluewake_pad_hold_until_released();
            bw_desktop::toggle_fullscreen();
            return true;
        case SDL_SCANCODE_F11:
            bw_desktop::toggle_fullscreen();
            return true;
        case SDL_SCANCODE_ESCAPE:
            if (bw_desktop::menu_open()) {
                bw_desktop::close_menu();
                return true;
            }
            // Esc with the mouse as the camera gives the mouse back first.
            if (!bluewake_mouse_camera_captured()) {
                bw_desktop::open_menu();
                return true;
            }
            return false;
        default:
            break;
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
            toggle_menu();
            return true;
        }
        break;
    default:
        break;
    }
    // While it is open the menu (ImGui) has the input to itself.
    return bw_desktop::menu_open();
}
