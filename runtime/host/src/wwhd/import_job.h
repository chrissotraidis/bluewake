// The built-in Wind Waker HD import as a background job, for the options menus
// (Mods > Wind Waker HD textures > Import from Wind Waker HD...): one import at a
// time on its own thread, with progress the menu reads every frame.
#pragma once

#include "pack.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace wwhd {

// What the player chose for each key: a key file, or a key they pasted into
// the form. Kept for this import only, never in settings.
struct ImportKeyFiles {
    std::string disc, title, common;
};

class ImportJob {
  public:
    // Finish cancellation before the job's state goes away at app shutdown.
    ~ImportJob();
    struct Status {
        bool running = false;
        bool finished = false; // a result is waiting (success or failure)
        bool failed = false;
        std::string message;   // progress, or the result in plain words
        std::string output;    // the new pack folder, on success
        size_t files_read = 0, files_total = 0, replacements = 0;
        // On success, what did not make it into the pack: textures whose name
        // already had a replacement, and HD resources that could not be read.
        size_t conflicts = 0, unsupported = 0;
    };

    // Starts an import of the Wind Waker HD dump at hd_picked (its folder or its
    // content folder, or image) with the GameCube disc gc_disc, into a new folder under
    // textures_dir. False (and a message in status) when it cannot start.
    bool start(const std::string& hd_picked, const std::string& gc_disc, const std::string& textures_dir,
               const ImportKeyFiles& keys = {});
    Status status() const;
    void cancel() { cancel_ = true; }
    // Takes a finished result once (so the menu applies it once).
    bool take_result(Status& result);

  private:
    mutable std::mutex lock_;
    Status status_;
    std::atomic<bool> cancel_{false};
    std::thread worker_;
};

ImportJob& import_job();

// The content folder of a Wind Waker HD dump: the folder itself, or its
// "content" subfolder.
std::string hd_content_folder(const std::string& picked);
// textures_dir/WWHD, or WWHD-2, WWHD-3... when taken.
std::string new_pack_folder(const std::string& textures_dir);

} // namespace wwhd
