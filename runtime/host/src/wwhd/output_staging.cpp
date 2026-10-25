#include "output_staging.h"
#include "cancel.h"
#include <array>
#include <cerrno>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace wwhd {
namespace fs = std::filesystem;
namespace {
constexpr const char* prefix = ".bluewake-wwhd-stage-v1-";
constexpr const char* lock_name = ".bluewake-wwhd-import.lock";
std::string marker(const std::string& id) { return "BlueWake WWHD output stage v1\n" + id + "\n"; }
[[noreturn]] void fail(const std::string& reason) {
    throw std::runtime_error("HD import temporary output: " + reason);
}
[[noreturn]] void os_error(const char* action) {
#ifdef _WIN32
    const std::error_code error(GetLastError(), std::system_category());
#else
    const std::error_code error(errno, std::generic_category());
#endif
    fail(std::string(action) + ": " + error.message());
}

// Separate opens must contend even in the same process. Never unlink this lock
// file: a second inode at the same name would permit two owners. Handles cannot
// be inherited by a launched program, and close/process death releases the lock.
class File {
public:
    enum Mode { lock, read, create };
    File(const fs::path& path, Mode mode) {
#ifdef _WIN32
        handle_ = CreateFileW(path.c_str(), mode == read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, mode == create ? CREATE_NEW : mode == read ? OPEN_EXISTING : OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) os_error("open ownership file");
        BY_HANDLE_FILE_INFORMATION info{};
        const bool valid = GetFileInformationByHandle(handle_, &info) &&
            GetFileType(handle_) == FILE_TYPE_DISK && info.nNumberOfLinks == 1 &&
            !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
#else
        const int flags = mode == read ? O_RDONLY : O_RDWR | O_CREAT | (mode == create ? O_EXCL : 0);
        handle_ = open(path.c_str(), flags | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (handle_ < 0) os_error("open ownership file");
        struct stat info{};
        const bool valid = fstat(handle_, &info) == 0 && S_ISREG(info.st_mode) && info.st_nlink == 1;
#endif
        if (!valid) { close(); fail("ownership file is not a plain, singly linked file"); }
    }
    ~File() { close(); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    void acquire() {
#ifdef _WIN32
        OVERLAPPED offset{};
        if (LockFileEx(handle_, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &offset)) return;
        if (GetLastError() == ERROR_LOCK_VIOLATION)
#else
        if (flock(handle_, LOCK_EX | LOCK_NB) == 0) return;
#if defined(__linux__) && defined(F_OFD_SETLK)
        // Android's emulated external storage rejects flock, but supports
        // open-file-description locks. These also exclude separate opens in
        // this process and remain owned until this handle closes.
        if (errno == ENOSYS || errno == EOPNOTSUPP) {
            struct flock range{};
            range.l_type = F_WRLCK;
            range.l_whence = SEEK_SET;
            range.l_len = 1;
            if (fcntl(handle_, F_OFD_SETLK, &range) == 0) return;
        }
#endif
        if (errno == EWOULDBLOCK || errno == EAGAIN)
#endif
            fail("another import is using this output folder; try again after it finishes");
        os_error("cannot lock output folder (no recovery performed)");
    }
    std::string contents() {
        std::array<char, 128> data{}; // A marker has a fixed, bounded size.
#ifdef _WIN32
        DWORD count = 0;
        if (!ReadFile(handle_, data.data(), DWORD(data.size()), &count, nullptr)) os_error("read ownership marker");
#else
        ssize_t count;
        do { count = ::read(handle_, data.data(), data.size()); } while (count < 0 && errno == EINTR);
        if (count < 0) os_error("read ownership marker");
#endif
        return std::string(data.data(), size_t(count));
    }
    void write_marker(const std::string& text) {
        size_t done = 0;
        while (done < text.size()) {
#ifdef _WIN32
            DWORD count = 0;
            if (!WriteFile(handle_, text.data() + done, DWORD(text.size() - done), &count, nullptr))
                os_error("write ownership marker");
#else
            ssize_t count;
            do { count = ::write(handle_, text.data() + done, text.size() - done); } while (count < 0 && errno == EINTR);
            if (count < 0) os_error("write ownership marker");
#endif
            if (!count) fail("short ownership marker write");
            done += size_t(count);
        }
        // Complete the marker before creating pack/. This is process-kill
        // recovery, not a promise of power-loss durability.
        if (!close()) os_error("close ownership marker");
    }
private:
    bool close() noexcept {
#ifdef _WIN32
        if (handle_ == INVALID_HANDLE_VALUE) return true;
        const HANDLE old = handle_; handle_ = INVALID_HANDLE_VALUE;
        return CloseHandle(old) != 0;
    }
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
        if (handle_ < 0) return true;
        const int old = handle_; handle_ = -1;
        return ::close(old) == 0;
    }
    int handle_ = -1;
#endif
};

bool plain(const fs::path& path, fs::file_type wanted) {
    std::error_code ec;
    const auto type = fs::symlink_status(path, ec).type();
    if (ec == std::errc::no_such_file_or_directory) return false;
    if (ec) fail("cannot inspect temporary output: " + ec.message());
    if (type != wanted) return false;
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) os_error("inspect temporary output");
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) return false; // includes junctions
#endif
    return true;
}

std::string path_utf8(const fs::path& path) {
    const auto text = path.u8string();
    return {text.begin(), text.end()}; // char in C++17, char8_t in app builds.
}
std::string id_of(const fs::path& path) {
    const std::string name = path_utf8(path.filename());
    const std::string head(prefix);
    if (name.size() != head.size() + 32 || name.compare(0, head.size(), head) != 0) return {};
    const std::string id = name.substr(head.size());
    return id.find_first_not_of("0123456789abcdef") == std::string::npos ? id : std::string();
}
bool payload_name(const fs::path& path) {
    const std::string name = path_utf8(path.filename());
    return name == "README.txt" || name == "import-report.json" ||
        (name.rfind("tex1_", 0) == 0 && path_utf8(path.extension()) == ".png");
}
// Validate the entire flat layout before deleting anything. Neither recovery
// nor ordinary cleanup traverses symlinks, junctions or unexpected subfolders.
bool owned(const fs::path& job, std::vector<fs::path>& files, bool& has_pack, bool recovering) {
    const std::string id = id_of(job);
    if (id.empty() || !plain(job, fs::file_type::directory) || !plain(job / "owner", fs::file_type::regular)) return false;
    if (fs::hard_link_count(job / "owner") != 1 || File(job / "owner", File::read).contents() != marker(id)) return false;
    has_pack = false;
    for (const auto& entry : fs::directory_iterator(job)) {
        if (recovering) check_cancel();
        if (entry.path().filename() == "owner") continue;
        if (entry.path().filename() != "pack" || !plain(entry.path(), fs::file_type::directory)) return false;
        has_pack = true;
        for (const auto& file : fs::directory_iterator(entry.path())) {
            if (recovering) check_cancel();
            if (!payload_name(file.path()) || !plain(file.path(), fs::file_type::regular)) return false;
            files.push_back(file.path());
        }
    }
    return true;
}
void remove_checked(const fs::path& path) {
    if (!fs::remove(path)) fail("temporary output changed during cleanup; retained for inspection");
}
bool cleanup(const fs::path& job, bool recovering) {
    std::vector<fs::path> files;
    bool has_pack = false;
    if (!owned(job, files, has_pack, recovering)) return false;
    for (const auto& file : files) {
        if (recovering) check_cancel();
        if (!plain(file, fs::file_type::regular)) fail("temporary output changed during cleanup");
        remove_checked(file);
        if (recovering) { WWHD_CHECKPOINT("recovery-file-removed"); }
    }
    if (recovering) check_cancel();
    if (has_pack) remove_checked(job / "pack");
    remove_checked(job / "owner"); // Last: a partial recovery remains recognizable.
    remove_checked(job);
    return true;
}
} // namespace

struct OutputStaging::State {
    fs::path output, parent, job;
    std::unique_ptr<File> lock;
};
OutputStaging::OutputStaging(const fs::path& output) : state_(std::make_unique<State>()) {
    check_cancel();
    const fs::path absolute = fs::absolute(output).lexically_normal();
    if (absolute.filename().empty()) fail("choose a new output folder");
    fs::create_directories(absolute.parent_path());
    state_->parent = fs::canonical(absolute.parent_path());
    state_->output = state_->parent / absolute.filename();
    // Let the actual nonblocking OS lock decide support. Filesystem labels do
    // not: Android external app data commonly lives on a local FUSE mount.
    state_->lock = std::make_unique<File>(state_->parent / lock_name, File::lock);
    state_->lock->acquire();
    // Check under the same lock as recovery and publication. Existing packs,
    // including dangling output symlinks, are never replacement destinations.
    if (fs::symlink_status(state_->output).type() != fs::file_type::not_found)
        fail("output already exists; choose a new folder to preserve existing packs");
    for (const auto& entry : fs::directory_iterator(state_->parent)) {
        check_cancel();
        if (!id_of(entry.path()).empty()) cleanup(entry.path(), true);
    }
}
OutputStaging::~OutputStaging() {
    try { discard(); }
    catch (const std::exception& error) { std::fprintf(stderr, "[wwhd] Temporary output retained: %s\n", error.what()); }
}
fs::path OutputStaging::create() {
    if (!state_->job.empty()) fail("temporary output already created");
    std::random_device random;
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        std::string id(32, '0');
        for (char& digit : id) digit = hex[random() & 15];
        const fs::path job = state_->parent / (std::string(prefix) + id);
        if (!fs::create_directory(job)) continue;
        state_->job = job;
#ifndef _WIN32
        if (chmod(job.c_str(), 0700) != 0) os_error("protect temporary output");
#endif
        File(job / "owner", File::create).write_marker(marker(id));
        WWHD_CHECKPOINT("staging-marker-ready");
        fs::create_directory(job / "pack");
        return job / "pack";
    }
    fail("cannot allocate a new temporary output folder");
}
void OutputStaging::discard() {
    if (state_->job.empty()) return;
    if (!cleanup(state_->job, false)) fail("unrecognized temporary output layout; files were kept");
    state_->job.clear();
}
void OutputStaging::publish() {
    check_cancel();
    if (fs::symlink_status(state_->output).type() != fs::file_type::not_found)
        fail("output already exists; choose a new folder to preserve existing packs");
    (fs::rename)(state_->job / "pack", state_->output);
    try {
#ifdef WWHD_IMPORT_TESTING
        import_test_checkpoint("staging-published"); // No cancellation check after commit.
#endif
        discard();
    }
    catch (const std::exception& error) {
        std::fprintf(stderr, "[wwhd] Pack complete; temporary wrapper retained: %s\n", error.what());
        state_->job.clear(); // Leave recovery to a later import; publication succeeded.
    }
}
} // namespace wwhd
