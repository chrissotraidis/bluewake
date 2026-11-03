// Same-process locking and commit/cleanup boundaries; generated files only.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "wwhd/output_staging.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <thread>

// Android external app data rejects flock. Exercise its fallback on Linux CI
// with real open-file-description locks and file operations; only flock's
// unsupported result is injected. The importer CLI tests normal flock too.
#if defined(__linux__) && !defined(__ANDROID__)
#include <cerrno>
extern "C" int flock(int, int) noexcept {
    errno = ENOSYS;
    return -1;
}
#endif

namespace fs = std::filesystem;
static void put(const fs::path& path, const char* text) {
    std::ofstream out(path); out << text; out.close(); assert(out);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const fs::path root = fs::absolute(argv[1]);
    assert(fs::create_directory(root)); // CTest supplies an unused, dedicated path.
    try {
        const fs::path first = root / "first", second = root / "second";
        const fs::path android_parent = root / "storage/emulated/0/Android/data/dev.bluewake.android/files/Load/Textures";
        { wwhd::OutputStaging external(android_parent / "WWHD");
          put(external.create() / "tex1_fixture.png", "Android external-files output"); external.publish(); }
        assert(fs::exists(android_parent / "WWHD/tex1_fixture.png"));
        {
            wwhd::OutputStaging owner(first / "complete");
            const fs::path payload = owner.create();
            put(payload / "tex1_fixture.png", "generated PNG stand-in");
            bool refused = false;
            std::thread contender([&] {
                try { wwhd::OutputStaging other(first / "different-name"); }
                catch (const std::exception& e) { refused = std::string(e.what()).find("another import") != std::string::npos; }
            });
            contender.join();
            assert(refused && fs::exists(payload / "tex1_fixture.png"));
            { wwhd::OutputStaging independent(second / "pack"); independent.create(); }
            assert(!fs::exists(second / "pack"));
            owner.publish();
            assert(fs::exists(first / "complete/tex1_fixture.png") && !fs::exists(payload.parent_path()));
            // The parent stays reserved until the import really returns.
            try { wwhd::OutputStaging other(first / "next"); assert(false); }
            catch (const std::exception& e) { assert(std::string(e.what()).find("another import") != std::string::npos); }
        }
        const fs::path lock = first / ".bluewake-wwhd-import.lock";
        const auto stamp = fs::last_write_time(lock);
        { wwhd::OutputStaging next(first / "next"); next.create(); }
        assert(fs::is_regular_file(lock) && fs::file_size(lock) == 0 && fs::last_write_time(lock) == stamp);
        {
            wwhd::OutputStaging committed(first / "published");
            const auto payload = committed.create();
            put(payload / "tex1_fixture.png", "complete");
            put(payload.parent_path() / "unexpected", "must be retained");
            committed.publish(); // Wrapper cleanup must not undo or misreport commit.
            assert(fs::exists(first / "published/tex1_fixture.png"));
            assert(fs::exists(payload.parent_path() / "owner"));
        }
        { wwhd::OutputStaging next(first / "later"); }
        assert(fs::exists(first / "complete/tex1_fixture.png") && fs::exists(first / "published/tex1_fixture.png"));
        // An invalid lock cannot be used as permission to reclaim anything.
        fs::create_directories(root / "bad-lock/.bluewake-wwhd-import.lock");
        put(root / "bad-lock/keep", "unrelated");
        const fs::path orphan = root / "bad-lock/.bluewake-wwhd-stage-v1-00000000000000000000000000000001";
        fs::create_directories(orphan / "pack");
        // Binary output preserves the exact marker on Windows too.
        { std::ofstream out(orphan / "owner", std::ios::binary);
          out << "BlueWake WWHD output stage v1\n00000000000000000000000000000001\n"; out.close(); assert(out); }
        put(orphan / "pack/tex1_keep.png", "recoverable, but not without a lock");
        try { wwhd::OutputStaging bad(root / "bad-lock/result"); assert(false); }
        catch (const std::exception&) {
            assert(fs::exists(root / "bad-lock/keep") && fs::exists(orphan / "owner") &&
                   fs::exists(orphan / "pack/tex1_keep.png"));
        }
        std::cout << "same-process contention, independent parents, lock lifetime, publication and refusal passed\n";
    } catch (...) { fs::remove_all(root); throw; }
    fs::remove_all(root);
}
