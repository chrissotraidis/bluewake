// The "Wind Waker HD textures" section of the desktop options menus: the Mac
// and Linux one (settings_menu.cpp) and the Windows one (win_settings.cpp).
// Both draw the section in their Mods tab, in the same order as the iPhone and
// iPad Mods menu (apple/ios/src/BWGameOverlay.mm), and count a pack the same way.
#pragma once

#include <SDL3/SDL.h>

#include <imgui.h>

#include <cctype>
#include <cstddef>
// The Windows host's POSIX shim (windows/compat/bw_posix_compat.h) defines
// rename() as a macro, which breaks <filesystem>'s own rename; set it aside here.
#pragma push_macro("rename")
#undef rename
#include <filesystem>
#pragma pop_macro("rename")
#include <mutex>
#include <string>
#include <system_error>

namespace bw_texture_pack {

// The replacement textures in a Dolphin-format pack: its .png and .dds files,
// leaving out the extra mip levels (name_mip1.png and so on), which belong to
// the texture before them.
inline size_t count(const std::string& folder) {
    namespace fs = std::filesystem;
    if (folder.empty())
        return 0;
    std::error_code ec;
    size_t textures = 0;
    fs::recursive_directory_iterator it(fs::path(folder), fs::directory_options::skip_permission_denied, ec);
    for (const fs::recursive_directory_iterator end; !ec && it != end; it.increment(ec)) {
        std::error_code type;
        if (!it->is_regular_file(type))
            continue;
        std::string ext = it->path().extension().string();
        for (char& c : ext)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        // A sidecar mip level (name_mip1.png, ..._mip10.dds) belongs to the
        // texture before it: skip a stem that ends in "_mip" + trailing digits,
        // the same rule the renderer's texture_replacement.cpp applies.
        std::string stem = it->path().stem().string();
        bool sidecar = false;
        if (stem.size() >= 5) {
            size_t i = stem.size();
            while (i > 0 && stem[i - 1] >= '0' && stem[i - 1] <= '9')
                --i;
            sidecar = i < stem.size() && i >= 4 && stem.compare(i - 4, 4, "_mip") == 0;
        }
        if ((ext == ".png" || ext == ".dds") && !sidecar)
            textures++;
    }
    return textures;
}

// count(), done again only when the folder changes or forget() is called (a
// pack holds thousands of files, and the menu draws every frame).
class Counter {
  public:
    size_t get(const std::string& folder) {
        if (!known_ || folder != folder_) {
            folder_ = folder;
            textures_ = count(folder);
            known_ = true;
        }
        return textures_;
    }
    void forget() { known_ = false; }

  private:
    std::string folder_;
    size_t textures_ = 0;
    bool known_ = false;
};

// What the menu can see of the pack without the renderer: the host counts the
// folder's textures itself, so this is the on-disk view, not the renderer's
// registration count (which needs a RecompCore query, #336). Green when the
// chosen folder holds textures, red when it does not, grey when no pack is on.
enum class PackState { Off, Empty, Ready };

inline PackState state_of(size_t textures, bool on) {
    if (!on)
        return PackState::Off;
    return textures > 0 ? PackState::Ready : PackState::Empty;
}

// What an import left out: textures whose name already had a replacement, and
// HD resources that could not be read. Amber, between the green of a pack that
// loaded and the red of one that did not.
inline ImVec4 skipped_color() { return ImVec4(1.f, 0.78f, 0.33f, 1.f); }

inline ImVec4 state_color(PackState s) {
    switch (s) {
    case PackState::Ready: return ImVec4(0.35f, 0.9f, 0.45f, 1.f); // green
    case PackState::Empty: return ImVec4(1.f, 0.4f, 0.35f, 1.f);   // red
    default:               return ImVec4(0.6f, 0.6f, 0.62f, 1.f);  // grey
    }
}

// System pickers may answer on another thread. Only this mailbox is shared;
// the menu consumes an answer before another dialog can replace its purpose.
enum class Pick { pack, wind_waker_hd, disc_image, disc_key, common_key, title_key };

struct Picker {
    std::mutex lock;
    bool showing = false;
    bool answered = false;
    Pick purpose = Pick::pack;
    std::string path;   // empty on cancellation or error
    std::string error;  // why the picker could not open ("" when it did)
};

inline Picker& picker() {
    // A late SDL callback must not reach a destroyed mutex at app shutdown.
    static Picker* p = new Picker;
    return *p;
}

inline bool begin_pick(Pick purpose) {
    Picker& p = picker();
    std::lock_guard<std::mutex> hold(p.lock);
    if (p.showing || p.answered)
        return false;
    p.showing = true;
    p.purpose = purpose;
    p.path.clear();
    p.error.clear();
    return true;
}

inline void SDLCALL picker_answer(void*, const char* const* files, int) {
    Picker& p = picker();
    std::lock_guard<std::mutex> hold(p.lock);
    if (files == nullptr) {
        p.error = SDL_GetError();
        if (p.error.empty())
            p.error = "this system has no file picker";
    } else if (files[0] != nullptr) {
        p.path = files[0]; // SDL frees these strings when the callback returns.
    }
    p.showing = false;
    p.answered = true; // Consume cancellation and errors as well as selections.
}

inline void choose_folder(SDL_Window* window, const std::string& start, Pick purpose = Pick::pack) {
    if (begin_pick(purpose))
        SDL_ShowOpenFolderDialog(picker_answer, nullptr, window, start.empty() ? nullptr : start.c_str(), false);
}

inline bool take_pick(std::string& path, std::string& error, Pick purpose) {
    Picker& p = picker();
    std::lock_guard<std::mutex> hold(p.lock);
    if (!p.answered || p.purpose != purpose)
        return false;
    p.answered = false;
    path = p.path;
    p.path.clear();
    error = p.error;
    return true;
}

// The folder the player picked for this purpose since the last call, if any.
inline bool take_folder(std::string& folder, Pick purpose = Pick::pack) {
    std::string error;
    return take_pick(folder, error, purpose) && !folder.empty();
}

inline bool picker_busy() {
    Picker& p = picker();
    std::lock_guard<std::mutex> hold(p.lock);
    return p.showing || p.answered;
}

// The existing menus display folder-picker errors themselves. Image/key errors
// belong to the import form, not their "type the texture-pack folder" fallback.
inline std::string picker_error() {
    Picker& p = picker();
    std::lock_guard<std::mutex> hold(p.lock);
    return p.purpose == Pick::pack || p.purpose == Pick::wind_waker_hd ? p.error : "";
}

// Import from a Wind Waker HD folder or WUD/WUX with local key files, followed
// by progress and Cancel. Key paths live only in the transient import form/job;
// they never enter settings. textures_dir is the app's Load/Textures folder.
// True once on success, with the new pack's folder in folder.
bool import_controls(SDL_Window* window, const std::string& textures_dir, std::string& folder);

} // namespace bw_texture_pack
