// The built-in Wind Waker HD import as a background job (import_job.h).
#include "import_job.h"
#include "filesystem.h"

#include <thread>

namespace wwhd {
namespace fs = std::filesystem;

ImportJob& import_job() {
    static ImportJob job;
    return job;
}

ImportJob::~ImportJob() {
    cancel();
    if (worker_.joinable())
        worker_.join();
}

std::string hd_content_folder(const std::string& picked) {
    std::error_code ec;
    const fs::path p(picked);
    if (fs::is_directory(p / "content", ec))
        return (p / "content").string();
    return picked;
}

std::string new_pack_folder(const std::string& textures_dir) {
    std::error_code ec;
    const fs::path base(textures_dir);
    for (int n = 1; n < 1000; n++) {
        const fs::path candidate = base / (n == 1 ? std::string("WWHD") : "WWHD-" + std::to_string(n));
        if (!fs::exists(candidate, ec))
            return candidate.string();
    }
    return (base / "WWHD-new").string();
}

bool ImportJob::start(const std::string& hd_picked, const std::string& gc_disc, const std::string& textures_dir,
                      const ImportKeyFiles& keys) {
    std::lock_guard<std::mutex> hold(lock_);
    if (status_.running)
        return false;
    // A finished worker has published its result and will not take lock_ again.
    if (worker_.joinable())
        worker_.join();
    status_ = Status();
    std::error_code ec;
    if (gc_disc.empty() || !fs::is_regular_file(fs::path(gc_disc), ec)) {
        status_.finished = status_.failed = true;
        status_.message = "BlueWake could not find the GameCube disc it plays, which the import also reads.";
        return false;
    }
    status_.running = true;
    status_.message = "Starting";
    cancel_ = false;
    ImportOptions options;
    options.hd_folder = hd_content_folder(hd_picked);
    options.disc_key_file = keys.disc;
    options.title_key_file = keys.title;
    options.common_key_file = keys.common;
    options.gc_disc = gc_disc;
    options.output = new_pack_folder(textures_dir);
    options.cancel = &cancel_;
    options.progress = [this](const ImportProgress& p) {
        std::lock_guard<std::mutex> hold(lock_);
        status_.files_read = p.files_read;
        status_.files_total = p.files_total;
        status_.replacements = p.replacements;
        status_.message = p.stage;
    };
    worker_ = std::thread([this, options] {
        Status done;
        done.finished = true;
        try {
            const ImportResult r = import_pack(options);
            done.output = options.output;
            done.replacements = r.replacements;
            done.conflicts = r.conflicts;
            done.unsupported = r.errors.size();
            done.message = "Imported " + std::to_string(r.replacements) + " textures from Wind Waker HD.";
        } catch (const std::exception& ex) {
            done.failed = true;
            done.message = cancel_ ? std::string("The import was cancelled.") : std::string("The import failed: ") + ex.what();
        }
        std::lock_guard<std::mutex> hold(lock_);
        done.files_read = status_.files_read;
        done.files_total = status_.files_total;
        status_ = done;
    });
    return true;
}

ImportJob::Status ImportJob::status() const {
    std::lock_guard<std::mutex> hold(lock_);
    return status_;
}

bool ImportJob::take_result(Status& result) {
    std::lock_guard<std::mutex> hold(lock_);
    if (!status_.finished)
        return false;
    result = status_;
    status_.finished = false;
    return true;
}

} // namespace wwhd
