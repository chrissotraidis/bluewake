// BlueWake's Linux first-run/configuration window: a setup wizard in the shape
// of the TotK port's installer (valentderah/totk-pc-recomp, installer/): a form,
// a preparing screen with its stage checklist, progress bar and log, and a ready
// screen. SDL owns the window and input, Dear ImGui supplies the widgets, and
// SDL's file dialogs use XDG Portal when the desktop provides it. Manual entry
// and drag-and-drop remain available.
//
// The preparation runs on an SDL thread so the loop keeps drawing; the importer
// reports its stage and fraction into Job, which the preparing screen renders.
#include "linux_setup.h"

#include "bluewake_version.h"
#include "disc_import.h"
#include "linux_disc.h"
#include "linux_setup_steps.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <imgui.h>

#include <atomic>
#include <cfloat>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <sys/statvfs.h>

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

// --- the preparation, on its own thread -------------------------------------

const char* const kStageName[BW_SETUP_STAGE_COUNT] = {
    "Checking the disc image and the data folder",
    "Reading the disc",
    "Unpacking the game modules",
    "Saving the settings",
};

// What the preparation writes: main.dol (~4 MB) and the 415 game modules (~7 MB).
constexpr unsigned long long kPreparationBytes = 12ull << 20;

double seconds_since(std::chrono::steady_clock::time_point then) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - then).count();
}

std::string free_space_in(const std::string& folder) {
    struct statvfs info {};
    if (folder.empty() || statvfs(folder.c_str(), &info) != 0) return {};
    const double free_bytes = static_cast<double>(info.f_bavail) * static_cast<double>(info.f_frsize);
    char text[512];
    std::snprintf(text, sizeof text, "Free space needed in %s: %llu MB (free: %.1f GB).",
                  folder.c_str(), kPreparationBytes >> 20, free_bytes / (1024.0 * 1024.0 * 1024.0));
    return text;
}

struct Job {
    enum State { Running = 0, Succeeded = 1, Failed = 2 };

    SDL_Thread* thread = nullptr;
    std::atomic<int> state{Running};
    std::atomic<int> stage{BW_SETUP_STAGE_CHECK};
    std::atomic<int> permille{0};   // the running stage's own progress, 0..1000
    std::mutex lock;                // guards the strings and the log
    std::string stage_text;         // the importer's own stage name, when it has one
    std::string error;
    std::vector<std::string> log;

    // The form's answers, copied before the thread starts.
    std::string data_dir, disc, textures, appimage, config;
    bool textures_enabled = false;
    bool install_shortcuts = false;
    std::map<std::string, std::string>* settings = nullptr;
    std::chrono::steady_clock::time_point started;
    int already_prepared = 0;

    void note(const std::string& line) {
        if (line.empty()) return;
        char stamp[32];
        std::snprintf(stamp, sizeof stamp, "[%5.1fs] ", seconds_since(started));
        std::lock_guard<std::mutex> guard(lock);
        log.push_back(std::string(stamp) + line);
    }

    // Also records the text in the log the first time a stage is entered.
    void set_stage(int index, double fraction, const char* text) {
        bool changed = false;
        {
            std::lock_guard<std::mutex> guard(lock);
            changed = stage_text != text;
            if (changed) stage_text = text;
        }
        stage.store(index);
        permille.store(static_cast<int>(fraction * 1000.0));
        if (changed) note(text);
    }

    // The importer's callback: which stage its report belongs to, how far along
    // that stage is, and its own words for the current task.
    void report(const char* name, double fraction) {
        if (fraction < 0.0) fraction = 0.0;
        if (fraction > 1.0) fraction = 1.0;
        if (name != nullptr && name[0] != '\0') {
            bool changed = false;
            {
                std::lock_guard<std::mutex> guard(lock);
                changed = stage_text != name;
                if (changed) stage_text = name;
            }
            if (changed) note(name);
        }
        stage.store(bw_setup_stage_for_report(name, fraction));
        permille.store(static_cast<int>(fraction * 1000.0));
    }

    double overall() const {
        const double within = state.load() == Succeeded ? 1.0 : permille.load() / 1000.0;
        return bw_setup_overall_fraction(stage.load(), within);
    }

    std::string current_task() {
        std::lock_guard<std::mutex> guard(lock);
        return stage_text;
    }

    std::vector<std::string> log_lines() {
        std::lock_guard<std::mutex> guard(lock);
        return log;
    }

    std::string failure() {
        std::lock_guard<std::mutex> guard(lock);
        return error;
    }

    void fail(const std::string& why) {
        {
            std::lock_guard<std::mutex> guard(lock);
            error = why;
        }
        note("failed: " + why);
        state.store(Failed);
    }

    // The settings file, the texture-pack variable the host reads, and the
    // application-menu shortcuts: the work the last stage covers.
    bool save_settings(std::string& why) {
        if (settings != nullptr) {
            (*settings)["DOL_AURORA_TEXTURE_PACK"] = textures_enabled ? textures : std::string();
            setenv("DOL_AURORA_TEXTURE_PACK", (*settings)["DOL_AURORA_TEXTURE_PACK"].c_str(), 1);
            if (config != "none" && !write_settings(config, *settings, why)) {
                why = "Could not save the settings: " + why;
                return false;
            }
        }
        if (install_shortcuts && !install_application_shortcuts(appimage, why)) {
            why = "Could not install the application shortcuts: " + why;
            return false;
        }
        return true;
    }
};

void report_progress(void* context, double fraction, const char* stage) {
    static_cast<Job*>(context)->report(stage, fraction);
}

int SDLCALL prepare_thread(void* data) {
    auto* job = static_cast<Job*>(data);
    job->note("checking " + job->disc);
    char error[1024] = "";
    if (bw_disc_prepare_into(job->data_dir.c_str(), job->disc.c_str(), report_progress, job,
                             &job->already_prepared, error, sizeof error) != 0) {
        job->fail(error[0] != '\0' ? error : "The disc image could not be prepared.");
        return 0;
    }
    job->note(job->already_prepared ? "the game files were already prepared"
                                    : "the game files are prepared");
    job->set_stage(BW_SETUP_STAGE_SAVE, 0.0, "Saving the settings");
    std::string why;
    if (!job->save_settings(why)) {
        job->fail(why);
        return 0;
    }
    job->set_stage(BW_SETUP_STAGE_SAVE, 1.0, "Ready");
    job->note("ready to play");
    job->state.store(Job::Succeeded);
    return 0;
}

// --- what the screens draw --------------------------------------------------

enum Mark { MarkPending, MarkRunning, MarkDone, MarkFailed };

const ImVec4 kDone(0.15f, 0.64f, 0.41f, 1.0f);
const ImVec4 kRunning(0.25f, 0.52f, 0.95f, 1.0f);
const ImVec4 kPending(0.55f, 0.55f, 0.55f, 1.0f);
const ImVec4 kFailed(0.85f, 0.23f, 0.20f, 1.0f);
const ImVec4 kNoticeBackground(0.24f, 0.20f, 0.06f, 1.0f);
const ImVec4 kNoticeText(1.00f, 0.86f, 0.45f, 1.0f);

// The tick/arrow/circle the checklist uses. Drawn rather than typed, because
// ImGui's default font has no glyphs for them.
void draw_mark(ImDrawList* list, ImVec2 centre, float radius, Mark mark) {
    const ImVec4& colour = mark == MarkDone      ? kDone
                           : mark == MarkFailed  ? kFailed
                           : mark == MarkRunning ? kRunning
                                                 : kPending;
    const ImU32 ink = ImGui::GetColorU32(colour);
    list->AddCircle(centre, radius, ink, 16, 1.6f);
    if (mark == MarkDone) {
        const float t = radius * 0.62f;
        list->AddLine(ImVec2(centre.x - t, centre.y), ImVec2(centre.x - t * 0.15f, centre.y + t * 0.75f), ink, 2.0f);
        list->AddLine(ImVec2(centre.x - t * 0.15f, centre.y + t * 0.75f), ImVec2(centre.x + t, centre.y - t * 0.8f), ink, 2.0f);
    } else if (mark == MarkFailed) {
        const float t = radius * 0.55f;
        list->AddLine(ImVec2(centre.x - t, centre.y - t), ImVec2(centre.x + t, centre.y + t), ink, 2.0f);
        list->AddLine(ImVec2(centre.x - t, centre.y + t), ImVec2(centre.x + t, centre.y - t), ink, 2.0f);
    } else if (mark == MarkRunning) {
        list->AddTriangleFilled(ImVec2(centre.x - radius * 0.35f, centre.y - radius * 0.6f),
                               ImVec2(centre.x - radius * 0.35f, centre.y + radius * 0.6f),
                               ImVec2(centre.x + radius * 0.65f, centre.y), ink);
    }
}

Mark mark_for(const Job& job, int index) {
    const int current = job.stage.load();
    const int state = job.state.load();
    if (state == Job::Failed && index == current) return MarkFailed;
    if (state == Job::Succeeded) return MarkDone;
    if (index < current) return MarkDone;
    if (index > current) return MarkPending;
    return MarkRunning;
}

void begin_bottom_row(const ImGuiStyle& style) {
    // The form is short and the preparation is long, so the action row sits on
    // the window's bottom edge on every screen, as the TotK installer's does.
    const float y = ImGui::GetWindowHeight() - ImGui::GetFrameHeight() - style.WindowPadding.y;
    ImGui::SetCursorPosY(y - style.ItemSpacing.y);
    ImGui::Separator();
    ImGui::SetCursorPosY(y);
}

void stage_row(const Job& job, int index) {
    const Mark mark = mark_for(job, index);
    const float height = ImGui::GetTextLineHeight();
    const float radius = height * 0.32f;
    const ImVec2 position = ImGui::GetCursorScreenPos();
    draw_mark(ImGui::GetWindowDrawList(),
              ImVec2(position.x + radius + 2.0f, position.y + height * 0.5f), radius, mark);
    ImGui::Dummy(ImVec2(radius * 2.0f + 10.0f, height));
    ImGui::SameLine();
    if (mark == MarkRunning) ImGui::TextColored(kRunning, "%s", kStageName[index]);
    else if (mark == MarkPending) ImGui::TextDisabled("%s", kStageName[index]);
    else if (mark == MarkFailed) ImGui::TextColored(kFailed, "%s", kStageName[index]);
    else ImGui::TextUnformatted(kStageName[index]);
}

void notice_box(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, kNoticeBackground);
    ImGui::PushStyleColor(ImGuiCol_Text, kNoticeText);
    ImGui::BeginChild("notice", ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 2.6f),
                      ImGuiChildFlags_Borders);
    ImGui::TextWrapped("%s", text);
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
}

void log_pane(Job& job) {
    ImGui::BeginChild("log", ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 7.0f),
                      ImGuiChildFlags_Borders);
    for (const std::string& line : job.log_lines()) ImGui::TextUnformatted(line.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
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
    SDL_Window* window = SDL_CreateWindow("BlueWake Setup", 900, 660,
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

    enum class Screen { Form, Preparing, Ready };
    Screen screen = Screen::Form;
    bool show_log = false;
    std::unique_ptr<Job> job;
    bool done = false, start = false;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            const bool preparing =
                job != nullptr && job->thread != nullptr && job->state.load() == Job::Running;
            // Closing the window while the disc is being prepared would leave
            // the preparation writing into a window that is gone; it finishes
            // first (a few seconds), then the window closes.
            if ((event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(window))) &&
                pick_kind == PickKind::None && !preparing) done = true;
            if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr &&
                screen == Screen::Form) {
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
        // A finished preparation keeps a failure on its own stage row and moves
        // a success on to the ready screen.
        if (job != nullptr && job->state.load() != Job::Running) {
            if (job->thread != nullptr) {
                SDL_WaitThread(job->thread, nullptr);
                job->thread = nullptr;
            }
            if (job->state.load() == Job::Succeeded) screen = Screen::Ready;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("BlueWake Setup", nullptr, ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("BlueWake Setup");
        ImGui::SameLine();
        ImGui::TextDisabled("version " BLUEWAKE_VERSION_STRING);
        ImGui::Separator();

        const ImGuiStyle& style = ImGui::GetStyle();
        if (screen == Screen::Form) {
            ImGui::TextWrapped("Plays The Legend of Zelda: The Wind Waker from your own GameCube disc. "
                               "BlueWake checks the disc and prepares its game files once.");
            ImGui::Spacing();
            notice_box("The folder this setup prepares holds the game's own files. "
                       "It is for your own use only; do not share or upload it.");
            ImGui::Spacing();

            if (ImGui::BeginTable("form", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings)) {
                ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("field", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Disc image");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-96.0f);
                if (ImGui::InputText("##disc", disc, disc_size)) validate_disc(disc, disc_status, disc_valid);
                ImGui::SameLine();
                ImGui::BeginDisabled(pick_kind != PickKind::None);
                if (ImGui::Button("Browse...##disc", ImVec2(88.0f, 0.0f))) {
                    static const SDL_DialogFileFilter filters[] = {{"GameCube disc images", "iso;gcm"}};
                    picker.result.store(0); pick_kind = PickKind::Disc; SDL_ClearError();
                    SDL_ShowOpenFileDialog(picked, &picker, window, filters, 1, nullptr, false);
                }
                ImGui::EndDisabled();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("HD texture pack");
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("Enable", &textures_enabled)) {
                    if (textures_enabled) validate_textures(textures, texture_status, textures_valid);
                    else { textures_valid = true; texture_status = "Optional"; }
                }
                ImGui::BeginDisabled(!textures_enabled);
                ImGui::SetNextItemWidth(-96.0f);
                if (ImGui::InputText("##textures", textures, sizeof textures))
                    validate_textures(textures, texture_status, textures_valid);
                ImGui::SameLine();
                ImGui::BeginDisabled(pick_kind != PickKind::None);
                if (ImGui::Button("Browse...##textures", ImVec2(88.0f, 0.0f))) {
                    picker.result.store(0); pick_kind = PickKind::Textures; SDL_ClearError();
                    SDL_ShowOpenFolderDialog(picked, &picker, window, nullptr, false);
                }
                ImGui::EndDisabled();
                ImGui::EndDisabled();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Shortcuts");
                ImGui::TableNextColumn();
                ImGui::BeginDisabled(!can_install_shortcuts);
                ImGui::Checkbox("Add or update application-menu and Desktop shortcuts", &install_shortcuts);
                ImGui::EndDisabled();
                ImGui::EndTable();
            }
            ImGui::TextColored(disc_valid ? kDone : ImVec4(1.f, .65f, .25f, 1.f), "%s", disc_status.c_str());
            if (textures_enabled)
                ImGui::TextDisabled("%s", texture_status.c_str());
            else
                ImGui::TextDisabled("Optional: a Dolphin-format pack's GZL or GZLE01 folder. "
                                    "You can also drag an ISO/GCM onto this window.");
            if (!message.empty()) ImGui::TextColored(kFailed, "%s", message.c_str());

            begin_bottom_row(style);
            constexpr float kStartWidth = 130.0f, kQuitWidth = 90.0f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x -
                                 kStartWidth - kQuitWidth - style.ItemSpacing.x);
            if (ImGui::Button("Quit##form", ImVec2(kQuitWidth, 0.0f))) done = true;
            ImGui::SameLine();
            ImGui::BeginDisabled(!disc_valid || !textures_valid);
            if (ImGui::Button("Start", ImVec2(kStartWidth, 0.0f))) {
                job = std::make_unique<Job>();
                job->data_dir = data_dir;
                job->disc = disc;
                job->textures_enabled = textures_enabled;
                job->textures = textures;
                job->install_shortcuts = install_shortcuts;
                job->appimage = appimage;
                job->config = config;
                job->settings = &settings;
                job->started = std::chrono::steady_clock::now();
                job->thread = SDL_CreateThread(prepare_thread, "bw-setup", job.get());
                if (job->thread == nullptr) {
                    message = std::string("The preparation could not start: ") + SDL_GetError();
                    job.reset();
                } else {
                    message.clear();
                    show_log = false;
                    screen = Screen::Preparing;
                }
            }
            ImGui::EndDisabled();
        } else if (screen == Screen::Preparing && job != nullptr) {
            ImGui::TextUnformatted("Preparing");
            ImGui::Separator();
            for (int index = 0; index < BW_SETUP_STAGE_COUNT; ++index) stage_row(*job, index);
            ImGui::Spacing();
            const bool failed = job->state.load() == Job::Failed;
            const std::string task = failed ? std::string() : job->current_task();
            if (!task.empty()) ImGui::TextDisabled("%s", task.c_str());
            char overlay[64];
            const double overall = job->overall();
            std::snprintf(overlay, sizeof overlay, "%d%%", static_cast<int>(overall * 100.0));
            ImGui::ProgressBar(static_cast<float>(overall), ImVec2(-FLT_MIN, 0.0f), overlay);
            const std::string space = free_space_in(data_dir);
            if (!space.empty()) ImGui::TextDisabled("%s", space.c_str());
            if (failed) ImGui::TextColored(kFailed, "%s", job->failure().c_str());
            if (show_log) log_pane(*job);

            begin_bottom_row(style);
            if (ImGui::Button("Show log##preparing", ImVec2(110.0f, 0.0f))) show_log = !show_log;
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 110.0f);
            if (failed) {
                if (ImGui::Button("Back", ImVec2(110.0f, 0.0f))) {
                    job.reset();
                    screen = Screen::Form;
                }
            } else {
                // The importer has no abort hook yet, so the preparation runs to
                // its end; a few seconds is shorter than recovering from a
                // half-written one.
                ImGui::BeginDisabled();
                ImGui::Button("Cancel", ImVec2(110.0f, 0.0f));
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("The game files are prepared once and this takes a few seconds, "
                                      "so the preparation finishes before the window closes.");
            }
        } else if (job != nullptr) {
            ImGui::TextUnformatted("Ready");
            ImGui::Separator();
            for (int index = 0; index < BW_SETUP_STAGE_COUNT; ++index) stage_row(*job, index);
            ImGui::Spacing();
            ImGui::TextWrapped("The game files are prepared. Later launches go straight to the game; "
                               "--setup reopens this window.");
            if (job->textures_enabled) ImGui::TextDisabled("HD texture pack: %s", job->textures.c_str());
            if (show_log) log_pane(*job);

            begin_bottom_row(style);
            if (ImGui::Button("Show log##ready", ImVec2(110.0f, 0.0f))) show_log = !show_log;
            constexpr float kPlayWidth = 150.0f, kQuitWidth = 90.0f;
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x -
                                 kPlayWidth - kQuitWidth - style.ItemSpacing.x);
            if (ImGui::Button("Quit##ready", ImVec2(kQuitWidth, 0.0f))) done = true;
            ImGui::SameLine();
            if (ImGui::Button("Start BlueWake", ImVec2(kPlayWidth, 0.0f))) {
                start = true;
                done = true;
            }
        }
        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 18, 23, 31, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    if (job != nullptr && job->thread != nullptr) SDL_WaitThread(job->thread, nullptr);
    job.reset();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    return start ? 1 : 0;
}
