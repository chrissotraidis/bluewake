// BlueWake's Linux first-run/configuration window. SDL owns the window and input,
// Dear ImGui supplies the widgets, and SDL's file dialogs use XDG Portal when
// the desktop provides it. Manual entry and drag-and-drop remain available.
#include "linux_setup.h"

#include "disc_import.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <imgui.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace {

namespace fs = std::filesystem;

struct Picker {
    std::atomic<int> result{0}; // 0 pending/idle, 1 selected, 2 cancelled, -1 error
    char path[4096]{};
    char error[1024]{};
};

void SDLCALL picked(void* userdata, const char* const* files, int) {
    auto* picker = static_cast<Picker*>(userdata);
    if (files == nullptr) {
        std::snprintf(picker->error, sizeof picker->error, "%s", SDL_GetError());
        picker->result.store(-1, std::memory_order_release);
    } else if (files[0] == nullptr) {
        picker->result.store(2, std::memory_order_release);
    } else {
        std::snprintf(picker->path, sizeof picker->path, "%s", files[0]);
        picker->result.store(1, std::memory_order_release);
    }
}

std::string settings_path() {
    if (const char* selected = std::getenv("BLUEWAKE_SETTINGS"))
        if (selected[0] != '\0') return selected;
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"))
        if (xdg[0] != '\0') return std::string(xdg) + "/BlueWake/settings.ini";
    if (const char* home = std::getenv("HOME"))
        return std::string(home) + "/.config/BlueWake/settings.ini";
    return {};
}

std::string xdg_data_home() {
    if (const char* xdg = std::getenv("XDG_DATA_HOME"))
        if (xdg[0] != '\0') return xdg;
    if (const char* home = std::getenv("HOME"))
        return std::string(home) + "/.local/share";
    return {};
}

std::string desktop_exec_quote(const std::string& path) {
    std::string quoted = "\"";
    for (const char character : path) {
        if (character == '%') {
            quoted += "%%";
            continue;
        }
        if (character == '\\' || character == '"' || character == '`' || character == '$')
            quoted += '\\';
        quoted += character;
    }
    return quoted + '"';
}

bool install_application_shortcuts(const std::string& appimage, std::string& error) {
    const std::string data_home = xdg_data_home();
    const char* appdir = std::getenv("APPDIR");
    if (data_home.empty()) { error = "HOME and XDG_DATA_HOME are not set"; return false; }
    if (appimage.empty()) { error = "APPIMAGE is not set; run setup from the AppImage"; return false; }

    std::error_code ec;
    const fs::path applications = fs::path(data_home) / "applications";
    const fs::path icons = fs::path(data_home) / "icons/hicolor/256x256/apps";
    fs::create_directories(applications, ec);
    if (ec) { error = ec.message(); return false; }
    fs::create_directories(icons, ec);
    if (ec) { error = ec.message(); return false; }

    const fs::path icon = icons / "dev.bluewake.BlueWake.png";
    if (appdir != nullptr && appdir[0] != '\0') {
        const fs::path bundled_icon = fs::path(appdir) / "BlueWake.png";
        if (fs::is_regular_file(bundled_icon, ec)) {
            fs::copy_file(bundled_icon, icon, fs::copy_options::overwrite_existing, ec);
            if (ec) { error = "could not install the BlueWake icon: " + ec.message(); return false; }
        }
    }

    if (!fs::is_regular_file(icon, ec)) {
        error = "the AppImage does not contain its application icon";
        return false;
    }

    const fs::path menu_entry = applications / "dev.bluewake.BlueWake.desktop";
    const fs::path pending = menu_entry.string() + ".setup.tmp";
    std::string executable;
    if (const char* extract = std::getenv("APPIMAGE_EXTRACT_AND_RUN"))
        if (extract[0] != '\0' && std::strcmp(extract, "0") != 0)
            executable = "env APPIMAGE_EXTRACT_AND_RUN=1 ";
    executable += desktop_exec_quote(appimage);
    // An absolute icon path is valid in a desktop entry and appears
    // immediately even when the desktop has not refreshed its icon-theme
    // cache yet. Keep installing it in hicolor as well for standard tooling.
    const std::string icon_value = icon.string();
    std::ofstream output(pending, std::ios::trunc);
    if (!output) { error = "could not create " + pending.string(); return false; }
    output << "[Desktop Entry]\n"
              "Type=Application\n"
              "Name=BlueWake\n"
              "Comment=The Legend of Zelda: The Wind Waker, statically recompiled\n"
              "Exec=" << executable << " %f\n"
              "Icon=" << icon_value << "\n"
              "Categories=Game;\n"
              "Terminal=false\n"
              "Actions=Setup;\n\n"
              "[Desktop Action Setup]\n"
              "Name=Configure BlueWake\n"
              "Exec=" << executable << " --setup\n";
    output.close();
    if (!output) { error = "could not write " + pending.string(); return false; }
    fs::permissions(pending, fs::perms::owner_read | fs::perms::owner_write |
                             fs::perms::group_read | fs::perms::others_read,
                    fs::perm_options::replace, ec);
    if (ec) { fs::remove(pending); error = ec.message(); return false; }
    fs::rename(pending, menu_entry, ec);
    if (ec) { fs::remove(pending); error = ec.message(); return false; }

    const char* desktop_path = SDL_GetUserFolder(SDL_FOLDER_DESKTOP);
    if (desktop_path == nullptr || desktop_path[0] == '\0') {
        error = std::string("could not find the XDG Desktop directory: ") + SDL_GetError();
        return false;
    }
    const fs::path desktop_dir = desktop_path;
    fs::create_directories(desktop_dir, ec);
    if (ec) { error = "could not create the Desktop directory: " + ec.message(); return false; }
    const fs::path desktop_entry = desktop_dir / "BlueWake.desktop";
    const fs::path desktop_pending = desktop_entry.string() + ".setup.tmp";
    fs::copy_file(menu_entry, desktop_pending, fs::copy_options::overwrite_existing, ec);
    if (ec) { error = "could not install the Desktop shortcut: " + ec.message(); return false; }
    fs::permissions(desktop_pending, fs::perms::owner_read | fs::perms::owner_write |
                                     fs::perms::owner_exec | fs::perms::group_read |
                                     fs::perms::group_exec | fs::perms::others_read |
                                     fs::perms::others_exec,
                    fs::perm_options::replace, ec);
    if (ec) { fs::remove(desktop_pending); error = ec.message(); return false; }
    fs::rename(desktop_pending, desktop_entry, ec);
    if (ec) { fs::remove(desktop_pending); error = ec.message(); return false; }
    return true;
}

std::map<std::string, std::string> read_settings(const std::string& path) {
    std::map<std::string, std::string> values;
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        const size_t equals = line.find('=');
        if (equals != std::string::npos && equals != 0 && line[0] != '#')
            values[line.substr(0, equals)] = line.substr(equals + 1);
    }
    return values;
}

bool write_settings(const std::string& path, const std::map<std::string, std::string>& values,
                    std::string& error) {
    if (path.empty()) { error = "HOME and XDG_CONFIG_HOME are not set"; return false; }
    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    if (ec) { error = ec.message(); return false; }
    const std::string pending = path + ".setup.tmp";
    std::ofstream output(pending, std::ios::trunc);
    if (!output) { error = "could not create " + pending; return false; }
    output << "# BlueWake settings, written by the Linux setup and in-game menus.\n";
    for (const auto& [key, value] : values) output << key << '=' << value << '\n';
    output.close();
    if (!output) { error = "could not write " + pending; return false; }
    fs::rename(pending, path, ec);
    if (ec) { fs::remove(pending); error = ec.message(); return false; }
    return true;
}

std::string value(const std::map<std::string, std::string>& values, const char* key,
                  const char* fallback = "") {
    const auto found = values.find(key);
    return found == values.end() ? fallback : found->second;
}

void copy_text(char* out, size_t size, const std::string& text) {
    std::snprintf(out, size, "%s", text.c_str());
}

bool validate_disc(const char* path, std::string& status, bool& valid) {
    valid = false;
    if (path[0] == '\0') { status = "Choose your GZLE01 revision-0 .iso or .gcm file."; return false; }
    char error[1024]{};
    if (bluewake_disc_check(path, error, sizeof error) != 0) {
        status = error[0] != '\0' ? error : "This disc image is not supported.";
        return false;
    }
    valid = true;
    status = "GZLE01 revision 0 - ready";
    return true;
}

bool validate_textures(const char* path, std::string& status, bool& valid) {
    valid = false;
    if (path[0] == '\0') {
        status = "Choose the texture pack's GZL or GZLE01 folder.";
        return false;
    }
    std::error_code ec;
    if (!fs::is_directory(path, ec)) {
        status = "The selected texture-pack folder does not exist.";
        return false;
    }
    valid = true;
    status = "Folder selected; textures load on game start.";
    return true;
}

} // namespace

extern "C" int bw_linux_setup(const char* data_dir, char* disc, size_t disc_size) {
    const std::string config = settings_path();
    auto settings = config == "none" ? std::map<std::string, std::string>{} : read_settings(config);

    if (disc[0] == '\0') {
        std::ifstream remembered(std::string(data_dir) + "disc.txt");
        std::string path;
        if (std::getline(remembered, path)) copy_text(disc, disc_size, path);
    }
    char textures[4096]{};
    copy_text(textures, sizeof textures, value(settings, "DOL_AURORA_TEXTURE_PACK"));
    bool textures_enabled = textures[0] != '\0';
    const char* appimage_env = std::getenv("APPIMAGE");
    const std::string appimage = appimage_env != nullptr ? appimage_env : "";
    const bool can_install_shortcuts = !appimage.empty();
    bool install_shortcuts = can_install_shortcuts;

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "[setup] SDL initialization failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Window* window = SDL_CreateWindow("BlueWake Setup", 820, 520,
                                           SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Renderer* renderer = window != nullptr ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (window == nullptr || renderer == nullptr) {
        std::fprintf(stderr, "[setup] window creation failed: %s\n", SDL_GetError());
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
        return -1;
    }
    SDL_SetRenderVSync(renderer, 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    Picker picker;
    enum class PickKind { None, Disc, Textures } pick_kind = PickKind::None;
    std::string disc_status;
    std::string texture_status = textures_enabled ? "" : "Optional";
    std::string message;
    bool disc_valid = false;
    bool textures_valid = !textures_enabled;
    if (disc[0] != '\0') validate_disc(disc, disc_status, disc_valid);
    else disc_status = "Choose your GZLE01 revision-0 .iso or .gcm file.";
    if (textures_enabled) validate_textures(textures, texture_status, textures_valid);

    bool done = false, start = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if ((event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(window))) &&
                pick_kind == PickKind::None) done = true;
            if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr) {
                copy_text(disc, disc_size, event.drop.data);
                validate_disc(disc, disc_status, disc_valid);
            }
        }
        const int picked_result = picker.result.exchange(0, std::memory_order_acquire);
        if (picked_result != 0) {
            if (picked_result == 1 && pick_kind == PickKind::Disc) {
                copy_text(disc, disc_size, picker.path);
                validate_disc(disc, disc_status, disc_valid);
            } else if (picked_result == 1 && pick_kind == PickKind::Textures) {
                copy_text(textures, sizeof textures, picker.path);
                textures_enabled = true;
                validate_textures(textures, texture_status, textures_valid);
            } else if (picked_result < 0) {
                message = std::string("The XDG file dialog failed: ") +
                          (picker.error[0] ? picker.error : SDL_GetError());
            }
            picker.path[0] = picker.error[0] = '\0';
            pick_kind = PickKind::None;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("BlueWake Setup", nullptr, ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("BlueWake Setup");
        ImGui::Separator();
        ImGui::TextWrapped("Choose your own USA GameCube Wind Waker disc. BlueWake validates and prepares it once.");
        if (ImGui::InputText("Disc image", disc, disc_size))
            validate_disc(disc, disc_status, disc_valid);
        ImGui::SameLine();
        ImGui::BeginDisabled(pick_kind != PickKind::None);
        if (ImGui::Button("Browse disc...")) {
            static const SDL_DialogFileFilter filters[] = {{"GameCube disc images", "iso;gcm"}};
            picker.result.store(0); pick_kind = PickKind::Disc; SDL_ClearError();
            SDL_ShowOpenFileDialog(picked, &picker, window, filters, 1, nullptr, false);
        }
        ImGui::EndDisabled();
        ImGui::TextColored(disc_valid ? ImVec4(.35f, .9f, .45f, 1.f) : ImVec4(1.f, .65f, .25f, 1.f),
                           "%s", disc_status.c_str());
        ImGui::TextDisabled("You can also drag an ISO/GCM onto this window.");
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        if (ImGui::Checkbox("Enable HD texture pack", &textures_enabled)) {
            if (textures_enabled) validate_textures(textures, texture_status, textures_valid);
            else { textures_valid = true; texture_status = "Optional"; }
        }
        ImGui::BeginDisabled(!textures_enabled);
        if (ImGui::InputText("Texture folder", textures, sizeof textures))
            validate_textures(textures, texture_status, textures_valid);
        ImGui::SameLine();
        ImGui::BeginDisabled(pick_kind != PickKind::None);
        if (ImGui::Button("Browse textures...")) {
            picker.result.store(0); pick_kind = PickKind::Textures; SDL_ClearError();
            SDL_ShowOpenFolderDialog(picked, &picker, window, nullptr, false);
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Select a Dolphin-format pack's GZL or GZLE01 folder.");
        ImGui::TextWrapped("%s", texture_status.c_str());
        ImGui::EndDisabled();
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::BeginDisabled(!can_install_shortcuts);
        ImGui::Checkbox("Add or update application-menu and Desktop shortcuts", &install_shortcuts);
        ImGui::EndDisabled();
        if (can_install_shortcuts)
            ImGui::TextDisabled("Keep the AppImage at its current path so the shortcut can find it.");
        else
            ImGui::TextDisabled("Application-menu and Desktop integration requires running setup from the AppImage.");
        if (!message.empty()) ImGui::TextColored(ImVec4(1.f, .4f, .35f, 1.f), "%s", message.c_str());
        ImGui::Separator();
        if (ImGui::Button("Quit")) done = true;
        ImGui::SameLine();
        ImGui::BeginDisabled(!disc_valid || !textures_valid);
        if (ImGui::Button("Start BlueWake")) {
            settings["DOL_AURORA_TEXTURE_PACK"] = textures_enabled ? textures : "";
            setenv("DOL_AURORA_TEXTURE_PACK", settings["DOL_AURORA_TEXTURE_PACK"].c_str(), 1);
            std::string error;
            if (config != "none" && !write_settings(config, settings, error))
                message = "Could not save settings: " + error;
            else if (install_shortcuts && !install_application_shortcuts(appimage, error))
                message = "Could not install the application shortcuts: " + error;
            else {
                start = true; done = true;
            }
        }
        ImGui::EndDisabled();
        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 18, 23, 31, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    return start ? 1 : 0;
}
