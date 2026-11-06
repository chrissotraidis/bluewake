// Import from Wind Waker HD, in the desktop menus' Mods tab (texture_pack_menu.h):
// picks the player's Wind Waker HD dump and runs the built-in importer
// (wwhd/import_job.h) in the background, with the disc BlueWake plays.
#include "texture_pack_menu.h"

#include "wwhd/disc_crypto.h"
#include "wwhd/import_job.h"

#include <imgui.h>

#include <cstdio>
#include <cstdlib>
#include <exception>

namespace bw_texture_pack {
namespace {

// Room for a key file's path as well as a pasted key.
constexpr size_t kKeyText = 1024;

struct ImageImport {
    bool active = false;
    bool use_title = false;
    std::string image;
    char disc_key[kKeyText] = {};
    char unlock_key[kKeyText] = {};
};

void choose_file(SDL_Window* window, const std::string& start, Pick purpose) {
    // SDL retains filters until its asynchronous callback. Never use stack
    // filters or a callback that captures the form or an ImGui window.
    static const SDL_DialogFileFilter images[] = {{"Wii U disc images", "wud;wux"}};
    static const SDL_DialogFileFilter keys[] = {{"Key files", "key;bin;txt"}, {"All files", "*"}};
    // A pasted key is not a folder to start in; only a path is.
    std::error_code ec;
    const std::string folder = start.empty() || !std::filesystem::is_regular_file(start, ec)
                                   ? std::string()
                                   : std::filesystem::path(start).parent_path().string();
    if (begin_pick(purpose))
        SDL_ShowOpenFileDialog(picker_answer, nullptr, window,
                               purpose == Pick::disc_image ? images : keys,
                               purpose == Pick::disc_image ? 1 : 2,
                               folder.empty() ? nullptr : folder.c_str(), false);
}

std::string filename(const std::string& path) {
    // SDL paths are UTF-8, including on Windows; avoid a locale conversion.
    const size_t slash = path.find_last_of("/\\");
    return path.empty() ? "No file selected" : path.substr(slash == std::string::npos ? 0 : slash + 1);
}

void file_control(SDL_Window* window, const char* label, const std::string& path, Pick purpose) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    if (ImGui::SmallButton(path.empty() ? "Choose file..." : "Change file..."))
        choose_file(window, path, purpose);
    ImGui::SameLine();
    ImGui::TextWrapped("%s", filename(path).c_str());
    ImGui::PopID();
}

// A key is either a local key file or 32 hex digits the player pasted; one
// field takes both, so a keys file and a copied key are the same gesture.
void key_control(SDL_Window* window, const char* label, const char* hint, char* value, size_t size,
                 Pick purpose) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::TextDisabled("paste a key, or choose a key file");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##key", value, size);
    if (ImGui::SmallButton("Choose a key file..."))
        choose_file(window, value, purpose);
    ImGui::TextDisabled("%s", hint);
    ImGui::PopID();
}

bool keys_readable(const std::string& source) {
    try {
        return !wwhd::key_candidates(source).empty();
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace

bool import_controls(SDL_Window* window, const std::string& textures_dir, std::string& folder) {
    static std::string message;
    static bool failed = false;
    // Kept beside the message, which outlives the job that made the pack.
    static size_t skipped_names = 0, skipped_unread = 0;
    static ImageImport form;
    wwhd::ImportJob& job = wwhd::import_job();

    std::string picked;
    if (take_folder(picked, Pick::wind_waker_hd)) {
        const char* disc = std::getenv("BLUEWAKE_DISC");
        message.clear();
        failed = false;
        skipped_names = skipped_unread = 0;
        job.start(picked, disc != nullptr ? disc : "", textures_dir);
    }

    for (const Pick purpose : {Pick::disc_image, Pick::disc_key, Pick::common_key, Pick::title_key}) {
        std::string error;
        if (!take_pick(picked, error, purpose) || !form.active)
            continue;
        if (!error.empty()) {
            message = "The file picker could not open. Please try again.";
            failed = true;
        } else if (!picked.empty()) {
            message.clear();
            failed = false;
            if (purpose == Pick::disc_image) {
                const size_t dot = picked.find_last_of('.');
                std::string extension = dot == std::string::npos ? "" : picked.substr(dot);
                for (char& c : extension)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (extension != ".wud" && extension != ".wux") {
                    message = "Choose a WUD or WUX disc image.";
                    failed = true;
                } else if (picked != form.image) {
                    form.image = picked;
                    // A different disc needs its own key; a pasted one went with the old disc.
                    form.disc_key[0] = '\0';
                    form.unlock_key[0] = '\0';
                }
            } else if (purpose == Pick::title_key) {
                std::snprintf(form.unlock_key, sizeof form.unlock_key, "%s", picked.c_str());
                form.use_title = true;
            } else if (purpose == Pick::common_key) {
                std::snprintf(form.unlock_key, sizeof form.unlock_key, "%s", picked.c_str());
                form.use_title = false;
            } else {
                std::snprintf(form.disc_key, sizeof form.disc_key, "%s", picked.c_str());
            }
        }
    }

    bool finished = false;
    wwhd::ImportJob::Status result;
    if (job.take_result(result)) {
        message = result.message;
        failed = result.failed;
        skipped_names = result.failed ? 0 : result.conflicts;
        skipped_unread = result.failed ? 0 : result.unsupported;
        if (!result.failed) {
            folder = result.output;
            finished = true;
            form = {}; // the pack is made; start again from the button
        }
        // On failure the form keeps the image and keys, so one changed key and
        // Import is the whole retry.
    }

    const wwhd::ImportJob::Status now = job.status();
    if (now.running) {
        const float done = now.files_total ? float(now.files_read) / float(now.files_total) : 0.f;
        char label[96];
        std::snprintf(label, sizeof label, "%zu of %zu files", now.files_read, now.files_total);
        ImGui::ProgressBar(done, ImVec2(-1.f, 0.f), label);
        ImGui::TextDisabled("%s: %zu textures so far. You can close the menu and play; it carries on.",
                            now.message.c_str(), now.replacements);
        if (ImGui::SmallButton("Cancel the import"))
            job.cancel();
        return finished;
    }

    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 34.0f);
    // Disable transitions until the current native dialog has answered. A
    // hidden/reopened menu can then safely consume its result on a later frame.
    ImGui::BeginDisabled(picker_busy());
    if (!form.active) {
        if (ImGui::Button("Import from Wind Waker HD..."))
            ImGui::OpenPopup("HD import source");
        if (ImGui::BeginPopup("HD import source")) {
            ImGui::TextUnformatted("Choose your Wind Waker HD input");
            if (ImGui::Selectable("Folder...")) {
                message.clear();
                failed = false;
                skipped_names = skipped_unread = 0;
                choose_folder(window, "", Pick::wind_waker_hd);
            }
            if (ImGui::Selectable("WUD/WUX image...")) {
                message.clear();
                failed = false;
                skipped_names = skipped_unread = 0;
                form = {};
                form.active = true;
                choose_file(window, "", Pick::disc_image);
            }
            ImGui::EndPopup();
        }
        ImGui::TextDisabled("Choose your Wind Waker HD dump folder (code, content and meta, or just content), "
                            "or a USA WUD/WUX image. BlueWake makes a texture pack using the GameCube disc it plays.");
    } else {
        file_control(window, "USA Wind Waker HD image", form.image, Pick::disc_image);
        key_control(window, "Disc key (required)", "A keys.txt for the emulators works as it is.", form.disc_key,
                    sizeof form.disc_key, Pick::disc_key);
        ImGui::TextUnformatted("Use one of these to unlock the title:");
        if (ImGui::RadioButton("Common key", !form.use_title)) {
            form.use_title = false;
            form.unlock_key[0] = '\0';
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Title key", form.use_title)) {
            form.use_title = true;
            form.unlock_key[0] = '\0';
        }
        key_control(window, form.use_title ? "Title key" : "Common key",
                    form.use_title ? "The disc's own title key." : "BlueWake decrypts the disc's ticket with it.",
                    form.unlock_key, sizeof form.unlock_key,
                    form.use_title ? Pick::title_key : Pick::common_key);
        ImGui::TextDisabled("Keys stay in this form and in the import; they are never saved in settings.");
        const bool filled = !form.image.empty() && form.disc_key[0] != '\0' && form.unlock_key[0] != '\0';
        ImGui::BeginDisabled(!filled);
        if (ImGui::Button("Import")) {
            const char* disc = std::getenv("BLUEWAKE_DISC");
            message.clear();
            failed = false;
            skipped_names = skipped_unread = 0;
            const std::string unlock(form.unlock_key);
            if (!keys_readable(form.disc_key) || !keys_readable(unlock)) {
                message = "Each key must be 32 hexadecimal digits, or a key file. A keys.txt file works as it is.";
                failed = true;
            } else {
                wwhd::ImportKeyFiles keys;
                keys.disc = form.disc_key;
                if (form.use_title)
                    keys.title = form.unlock_key;
                else
                    keys.common = form.unlock_key;
                job.start(form.image, disc != nullptr ? disc : "", textures_dir, keys);
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            form = {};
            message.clear();
            failed = false;
            skipped_names = skipped_unread = 0;
        }
    }
    ImGui::EndDisabled();
    if (!message.empty()) {
        // What was imported in the pack state's green, what was left out in
        // amber, a failure in its red.
        ImGui::TextColored(state_color(failed ? PackState::Empty : PackState::Ready), "%s", message.c_str());
        if (!failed && skipped_names)
            ImGui::TextColored(skipped_color(), "%zu skipped: another texture already has that name.", skipped_names);
        if (!failed && skipped_unread)
            ImGui::TextColored(skipped_color(), "%zu skipped: could not be read; import-report.json lists them.",
                               skipped_unread);
    }
    ImGui::PopTextWrapPos();
    return finished;
}

} // namespace bw_texture_pack
