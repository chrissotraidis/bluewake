// SPDX-License-Identifier: GPL-3.0-or-later
// Dolphin saves in the desktop menu (dolphin_save_menu.h).
//
// The running game keeps its card in memory and writes all of it on every
// save, so an import holds the game's card calls (they answer busy), puts the
// new card in place and restarts BlueWake, or closes it where it cannot start
// itself again (the Mac app): the imported save then loads with the game. An
// export only reads the card file.
//
// The card's own paths are opened with fopen, as the card itself is
// (memory_card.c); the files the player picks come from SDL as UTF-8 and go
// through SDL's file calls, which take UTF-8 on Windows too.
#include "dolphin_save_menu.h"

#include "../../../apple/ios/src/dolphin_save_import.h"
#include "atomic_file.h"
#include "desktop_menu.h"
extern "C" {
#include "card_runtime.h"  // a C header without its own guards
}
#include "texture_pack_menu.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace bw_dolphin_save {
namespace {

using bw_texture_pack::Pick;
using Bytes = std::vector<uint8_t>;

// An import the player is choosing.
struct Import {
    bool active = false;
    std::string name;  // the Dolphin file's name, for the messages
    Bytes save;        // its 64-byte directory entry, then the gczelda file: a .gci's layout
    BWQuestLog theirs[BW_QUEST_LOGS] = {};
    bool has_saves = false;  // BlueWake's card holds a gczelda file
    BWQuestLog here[BW_QUEST_LOGS] = {};
    int source = 0;       // 1-3; 0 until chosen
    int destination = 0;  // 1-3; 0 until chosen
};

Import g_import;
std::string g_message;
bool g_message_bad = false;
bool g_closing = false;  // imported: the game's card calls stay held until it restarts

void say(const std::string& message, bool bad) {
    g_message = message;
    g_message_bad = bad;
}

bool read_card(const char* path, Bytes& out) {
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr)
        return false;
    out.clear();
    uint8_t buffer[65536];
    size_t count;
    while ((count = std::fread(buffer, 1, sizeof buffer, file)) != 0)
        out.insert(out.end(), buffer, buffer + count);
    const bool ok = !std::ferror(file);
    std::fclose(file);
    return ok;
}

bool write_flushed(const std::string& path, const uint8_t* data, size_t size) {
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr)
        return false;
    bool ok = std::fwrite(data, 1, size, file) == size && bw_atomic_flush(file);
    if (std::fclose(file) != 0)
        ok = false;
    if (!ok)
        std::remove(path.c_str());
    return ok;
}

bool load_picked(const std::string& path, Bytes& out) {
    size_t size = 0;
    void* data = SDL_LoadFile(path.c_str(), &size);
    if (data == nullptr)
        return false;
    out.assign(static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
    SDL_free(data);
    return true;
}

std::string file_name(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

// Where the player keeps Dolphin saves: the folder of the last file picked to
// import or saved by an export (UTF-8, as SDL gives it), kept in the settings
// file. Both pickers open there.
constexpr const char* kFolderKey = "BLUEWAKE_DOLPHIN_SAVE_FOLDER";

std::string remembered_folder() {
    const std::string folder = bw_desktop::env(kFolderKey);
    SDL_PathInfo info;
    return !folder.empty() && SDL_GetPathInfo(folder.c_str(), &info) && info.type == SDL_PATHTYPE_DIRECTORY
               ? folder
               : std::string();
}

void remember_folder_of(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos)
        return;
    const std::string folder = path.substr(0, slash == 2 && path[1] == ':' ? 3 : slash);  // keep "C:\"
    if (folder != bw_desktop::env(kFolderKey))
        bw_desktop::set_env(kFolderKey, folder);
}

std::string now(const char* format) {
    const std::time_t t = std::time(nullptr);
    char text[32] = {};
    std::strftime(text, sizeof text, format, std::localtime(&t));
    return text;
}

// "Link · 6¾ hearts · 180 rupees", as the iPhone app says it.
std::string summary(const BWQuestLog& log) {
    static const char* const quarters[] = {"", "\xC2\xBC", "\xC2\xBD", "\xC2\xBE"};
    const unsigned whole = log.max_life / 4;
    char text[96];
    std::snprintf(text, sizeof text, "%s \xC2\xB7 %u%s heart%s \xC2\xB7 %u rupee%s",
                  log.name[0] != 0 ? log.name : "No name", whole, quarters[log.max_life % 4],
                  log.max_life == 4 ? "" : "s", log.rupees, log.rupees == 1 ? "" : "s");
    return text;
}

// The card's folder's Backups, where the phone apps keep the card an import replaced.
std::filesystem::path backups_folder(const char* card) {
    return std::filesystem::path(card).parent_path() / "Backups";
}

// --- import -------------------------------------------------------------------

void choose_import(SDL_Window* window) {
    // SDL keeps the filters until its callback, so they are static.
    static const SDL_DialogFileFilter filters[] = {{"Dolphin saves (.gci, .raw)", "gci;raw"}, {"All files", "*"}};
    static std::string start;
    start = remembered_folder();
    if (bw_texture_pack::begin_pick(Pick::dolphin_save))
        SDL_ShowOpenFileDialog(bw_texture_pack::picker_answer, nullptr, window, filters, 2,
                               start.empty() ? nullptr : start.c_str(), false);
}

void read_import(const std::string& path, const char* card_path) {
    Bytes file, card;
    BWDolphinSave save = {};
    const char* error = !load_picked(path, file) ? "The file could not be read."
                        : !read_card(card_path, card)
                            ? "BlueWake's memory card could not be read."
                            : bw_dolphin_save_parse(file.data(), file.size(), &save);
    Import next;
    if (error == nullptr)
        error = bw_card_quest_logs(card.data(), card.size(), &next.has_saves, next.here);
    if (error != nullptr) {
        bw_dolphin_save_free(&save);
        say(std::string("Could not import: ") + error, true);
        return;
    }
    bw_gczelda_quest_logs(save.data, next.theirs);
    next.save.assign(save.entry, save.entry + sizeof save.entry);
    next.save.insert(next.save.end(), save.data, save.data + save.length);
    bw_dolphin_save_free(&save);
    for (int i = 0; i < BW_QUEST_LOGS && next.source == 0; i++)
        if (!next.theirs[i].empty && next.theirs[i].checksum_ok)
            next.source = i + 1;
    if (next.source == 0) {
        say("Nothing to import: " + file_name(path) + " has no quest logs BlueWake can read.", true);
        return;
    }
    // An empty quest log is the safe first choice; replacing one is always picked by hand.
    for (int i = 0; i < BW_QUEST_LOGS && next.destination == 0; i++)
        if (next.here[i].empty)
            next.destination = i + 1;
    next.name = file_name(path);
    next.active = true;
    g_import = std::move(next);
    g_message.clear();
}

void finish_import(const char* card_path) {
    const Import& in = g_import;
    // The card is read again in case the game saved while the menu was up.
    Bytes card;
    BWDolphinSave save = {};
    uint8_t* bytes = nullptr;
    size_t size = 0;
    const int source = in.has_saves ? in.source : 0;
    const int destination = in.has_saves ? in.destination : 0;
    const char* error = !read_card(card_path, card) ? "BlueWake's memory card could not be read."
                                                    : bw_dolphin_save_parse(in.save.data(), in.save.size(), &save);
    if (error == nullptr)
        error = bw_card_import(card.data(), card.size(), &save, source, destination, &bytes, &size);
    bw_dolphin_save_free(&save);
    if (error != nullptr) {
        say(std::string("Could not import: ") + error, true);
        return;
    }

    // From here the game's card calls answer busy, so its next save cannot put
    // the old card back over the import.
    bluewake_card_runtime_suspend_writes(true);
    const std::string stamp = now("%Y%m%d-%H%M%S");
    const std::filesystem::path backups = backups_folder(card_path);
    const std::string backup = (backups / ("GZLE01-" + stamp + ".card")).string();
    const std::string staged = std::string(card_path) + ".import-" + stamp;
    std::error_code ec;
    std::filesystem::create_directories(backups, ec);
    const char* failed = nullptr;
    if (!write_flushed(backup, card.data(), card.size()))
        failed = "Your current saves could not be backed up, so nothing was changed.";
    else if (!write_flushed(staged, bytes, size) || !dol_card_validate(staged.c_str()))
        failed = "The new card could not be written, so your saves were not changed.";
    // Unqualified: on Windows rename is the compat shim's, which replaces an existing file.
    else if (rename(staged.c_str(), card_path) != 0)
        failed = "The new card could not be put in place, so your saves were not changed.";
    std::free(bytes);
    if (failed != nullptr) {
        std::remove(staged.c_str());
        bluewake_card_runtime_suspend_writes(false);
        say(failed, true);
        return;
    }
    std::fprintf(stderr, "[dolphin-save] imported %s (quest log %d into %d); the card before is %s\n",
                 in.name.c_str(), source, destination, backup.c_str());
    g_import = Import();
    g_closing = true;
    if (bw_platform::can_restart()) {
        say("Save imported. BlueWake is restarting so it loads.", false);
        bw_platform::restart();
    } else {
        say("Save imported. BlueWake is closing: open it again and pick the quest log to play.", false);
        bw_desktop::save();
        SDL_Event quit{};
        quit.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit);
    }
}

void draw_import(const char* card_path) {
    Import& in = g_import;
    ImGui::Text("Import from %s", in.name.c_str());
    ImGui::TextDisabled(in.has_saves ? "Quest log to bring in:" : "Its quest logs:");
    for (int i = 0; i < BW_QUEST_LOGS; i++) {
        const BWQuestLog& log = in.theirs[i];
        if (log.empty)
            continue;
        const std::string label = summary(log) + (log.checksum_ok ? "" : " (damaged, cannot be imported)");
        ImGui::PushID(i);
        ImGui::BeginDisabled(!log.checksum_ok || !in.has_saves);
        if (ImGui::Selectable(label.c_str(), in.has_saves && in.source == i + 1))
            in.source = i + 1;
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    std::string what;
    std::string go = "Import";
    if (!in.has_saves) {
        what = "BlueWake has no saves yet, so every quest log in " + in.name + " is copied in.";
    } else {
        ImGui::Spacing();
        ImGui::TextDisabled("Put it in:");
        for (int i = 0; i < BW_QUEST_LOGS; i++) {
            const std::string label = "Quest Log " + std::to_string(i + 1) + ": " +
                                      (in.here[i].empty ? std::string("Empty") : summary(in.here[i]));
            ImGui::PushID(BW_QUEST_LOGS + i);
            if (ImGui::Selectable(label.c_str(), in.destination == i + 1))
                in.destination = i + 1;
            ImGui::PopID();
        }
        if (in.source != 0 && in.destination != 0) {
            const BWQuestLog& there = in.here[in.destination - 1];
            const std::string theirs = summary(in.theirs[in.source - 1]);
            if (there.empty) {
                what = theirs + " goes into Quest Log " + std::to_string(in.destination) + ".";
            } else {
                what = "Quest Log " + std::to_string(in.destination) + " (" + summary(there) + ") is replaced by " +
                       theirs + ".";
                go = "Replace Quest Log " + std::to_string(in.destination);
            }
        } else {
            what = "Pick the quest log to put it in.";
        }
    }
    ImGui::Spacing();
    ImGui::TextWrapped("%s A copy of your current saves is kept first, in %s.", what.c_str(),
                       backups_folder(card_path).string().c_str());
    ImGui::TextWrapped(bw_platform::can_restart()
                           ? "Do this at the title screen or right after saving: BlueWake then restarts, and the "
                             "imported save loads with the game."
                           : "Do this at the title screen or right after saving: BlueWake then closes, and the "
                             "imported save loads when you open it again.");
    const bool ready = !in.has_saves || (in.source != 0 && in.destination != 0);
    ImGui::BeginDisabled(!ready);
    if (ImGui::Button(go.c_str()))
        finish_import(card_path);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        g_import = Import();
}

// --- export -------------------------------------------------------------------

void choose_export(SDL_Window* window, const char* card_path) {
    static const SDL_DialogFileFilter filters[] = {{"Dolphin saves (.gci)", "gci"}};
    // A file name to start with, in the remembered folder, or the card's before there is one.
    static std::string start;
    const std::string name = "GZLE01-" + now("%Y-%m-%d") + ".gci";
    const std::string folder = remembered_folder();
    start = folder.empty() ? (std::filesystem::path(card_path).parent_path() / name).string()
                           : folder + (folder.back() == '/' || folder.back() == '\\' ? "" : "/") + name;
    if (bw_texture_pack::begin_pick(Pick::dolphin_export))
        SDL_ShowSaveFileDialog(bw_texture_pack::picker_answer, nullptr, window, filters, 1, start.c_str());
}

void write_export(std::string path, const char* card_path) {
    if (std::filesystem::path(file_name(path)).extension().empty())
        path += ".gci";
    Bytes card;
    uint8_t* bytes = nullptr;
    size_t size = 0;
    const char* error = !read_card(card_path, card) ? "BlueWake's memory card could not be read."
                                                    : bw_card_export(card.data(), card.size(), &bytes, &size);
    if (error == nullptr && !SDL_SaveFile(path.c_str(), bytes, size))
        error = "The file could not be written.";
    std::free(bytes);
    if (error != nullptr) {
        say(std::string("Could not export: ") + error, true);
        return;
    }
    std::fprintf(stderr, "[dolphin-save] exported the card to %s\n", path.c_str());
    say("Saved " + file_name(path) +
            ": all three quest logs, for Dolphin's Memory Card Manager (Tools > Memory Card Manager > Import). "
            "Your own saves are unchanged.",
        false);
}

}  // namespace

void controls(SDL_Window* window) {
    ImGui::SeparatorText("Dolphin saves");
    const char* card = bluewake_card_runtime_path();
    if (card == nullptr) {
        ImGui::TextDisabled("This session has no memory card, so there is nothing to import into or export.");
        return;
    }
    std::string path, error;
    if (bw_texture_pack::take_pick(path, error, Pick::dolphin_save)) {
        if (!path.empty()) {
            remember_folder_of(path);
            read_import(path, card);
        }
        else if (!error.empty())
            say("The file picker did not open (" + error + ").", true);
    }
    if (bw_texture_pack::take_pick(path, error, Pick::dolphin_export)) {
        if (!path.empty()) {
            remember_folder_of(path);
            write_export(path, card);
        }
        else if (!error.empty())
            say("The file picker did not open (" + error + ").", true);
    }

    if (g_import.active && !g_closing) {
        draw_import(card);
    } else {
        ImGui::TextDisabled("Bring a quest log in from a Dolphin .gci or memory card (.raw), or write yours out "
                            "as a .gci that Dolphin opens.");
        ImGui::BeginDisabled(g_closing || bw_texture_pack::picker_busy());
        if (ImGui::Button("Import Dolphin Save..."))
            choose_import(window);
        ImGui::SameLine();
        if (ImGui::Button("Export Dolphin Save..."))
            choose_export(window, card);
        ImGui::EndDisabled();
    }
    if (!g_message.empty()) {
        if (g_message_bad)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.55f, 0.45f, 1.f));
        ImGui::TextWrapped("%s", g_message.c_str());
        if (g_message_bad)
            ImGui::PopStyleColor();
    }
}

}  // namespace bw_dolphin_save
