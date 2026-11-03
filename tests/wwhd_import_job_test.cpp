// Exercise the real background-job lifetime with a controllable import worker.
// No game data, renderer or texture decoding is involved.
#include "wwhd/import_job.h"
#include "wwhd/filesystem.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <thread>

namespace {
enum Mode { succeed, fail, wait_for_cancel };
std::atomic<Mode> mode{succeed};
std::atomic<bool> entered{false}, exited{false};
wwhd::ImportKeyFiles expected_keys;

void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

template <typename Predicate> void until(Predicate ready) {
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!ready()) {
        require(std::chrono::steady_clock::now() < limit, "worker timed out");
        std::this_thread::yield();
    }
}
} // namespace

namespace wwhd {
ImportResult import_pack(const ImportOptions& options) {
    require(options.disc_key_file == expected_keys.disc && options.title_key_file == expected_keys.title &&
                options.common_key_file == expected_keys.common, "selected key paths were lost or changed");
    entered = true;
    options.progress({1, 2, 0, "fixture"});
    if (mode == wait_for_cancel)
        until([&] { return options.cancel->load(); });
    exited = true;
    if (options.cancel->load() || mode == fail)
        throw FormatError("fixture stopped");
    ImportResult result;
    result.replacements = 3;
    return result;
}
} // namespace wwhd

int main() {
    namespace fs = std::filesystem;
    const auto serial = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path dir = fs::temp_directory_path() / ("bluewake-import-job-" + std::to_string(serial));
    try {
        require(fs::create_directory(dir), "fixture directory already exists");
        const auto disc = (dir / "fixture.iso").string();
        std::ofstream(disc) << "generated test input";
        {
            wwhd::ImportJob job;
            for (const Mode next : {succeed, fail, wait_for_cancel}) {
                mode = next;
                entered = exited = false;
                require(job.start(dir.string(), disc, dir.string()), "could not restart worker");
                until([] { return entered.load(); });
                if (next == wait_for_cancel) {
                    require(!job.start(dir.string(), disc, dir.string()), "started two imports");
                    job.cancel();
                }
                until([&] { return job.status().finished; });
                wwhd::ImportJob::Status result;
                require(job.take_result(result), "missing result");
                require(result.failed == (next != succeed), "incorrect result status");
                require(next != succeed || result.replacements == 3, "lost replacement count");
                require(!job.take_result(result), "delivered result twice");
            }
            // The background job owns its selection even if the menu resets
            // immediately after Import is pressed.
            mode = succeed;
            expected_keys = {"synthetic-disc.key", "synthetic-title.key", ""};
            auto selected = expected_keys;
            require(job.start(disc, disc, dir.string(), selected), "could not start image import");
            selected = {};
            until([&] { return job.status().finished; });
            require(!job.status().failed, "image import did not preserve its key selection");
            expected_keys = {};
            require(!job.start(dir.string(), "", dir.string()), "accepted missing disc");
            require(job.status().failed && !job.status().running, "missing-disc state incorrect");
        }
        mode = wait_for_cancel;
        entered = exited = false;
        {
            wwhd::ImportJob job;
            require(job.start(dir.string(), disc, dir.string()), "could not start shutdown fixture");
            until([] { return entered.load(); });
        }
        require(exited.load(), "job was destroyed while its worker still used it");
        fs::remove_all(dir);
        std::puts("Import job completion, cancellation, restart and shutdown passed");
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        // Leave only generated fixture data for inspection after a failure.
        return 1;
    }
}
