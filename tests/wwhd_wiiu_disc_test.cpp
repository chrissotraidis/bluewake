// Full-flow native disc tests. Generate input with tests/wwhd_disc_fixture.py;
// argv[1] is its fixture directory. No parser/crypto implementation lives here.
#include "wwhd/wiiu_disc.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace {
namespace fs = std::filesystem;
using wwhd::Bytes;
using wwhd::DiscKey;
using wwhd::WiiUDisc;
using wwhd::WiiUDiscEntry;
using wwhd::WiiUDiscKeys;

void check(bool ok, const std::string& message) {
    if (!ok)
        throw std::runtime_error(message);
}

template <typename Fn> void rejected(Fn action, const std::string& label) {
    try {
        action();
    } catch (const wwhd::FormatError&) {
        return;
    }
    throw std::runtime_error(label + ": expected FormatError");
}

DiscKey synthetic(const char* text) {
    check(std::char_traits<char>::length(text) == 16, "synthetic key must have 16 bytes");
    DiscKey result{};
    std::copy_n(text, result.size(), result.begin());
    return result;
}

Bytes load(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    check(bool(file), "missing generated fixture: " + path.string());
    Bytes data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    check(!file.bad(), "cannot read generated fixture: " + path.string());
    return data;
}

const WiiUDiscEntry expected[] = {
    {"content/raw-head.bin", 0, 41, 1, 0},
    {"content/raw-cross.bin", 32763, 87, 1, 0x40},
    {"content/raw-empty.bin", 65535, 0, 1, 0},
    {"content/cbc-head.bin", 0, 49, 2, 0},
    {"content/cbc-unaligned.bin", 15, 77, 2, 0},
    {"content/cbc-cross.bin", 32763, 97, 2, 0},
    {"content/cbc-tail.bin", 65520, 15, 2, 0},
    {"content/cbc-empty.bin", 65535, 0, 2, 0},
    {"content/hashed-head.bin", 0, 19, 3, 0},
    {"content/hashed-cross.bin", 0xFC00 - 6, 0xFC00 + 23, 3, 0},
    {"content/nested/hashed-block1.bin", 0xFC00 + 21, 71, 3, 0},
    {"content/nested/hashed-tail.bin", 3 * 0xFC00 - 15, 15, 3, 0},
    {"content/nested/hashed-empty.bin", 3 * 0xFC00, 0, 3, 0},
    {"outside.bin", 123, 15, 1, 0},
};

void positive(const fs::path& root, const char* name, const WiiUDiscKeys& keys) {
    WiiUDisc disc((root / name).string(), keys);
    const auto& entries = disc.entries();
    check(entries.size() == std::size(expected), std::string(name) + ": entry count");
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& got = entries[i];
        const auto& want = expected[i];
        check(got.path == want.path && got.offset == want.offset && got.size == want.size &&
                  got.cluster == want.cluster && got.flags == want.flags,
              std::string(name) + ": entry metadata/order differs at " + std::to_string(i));
    }
    // Reverse and forward passes revisit hashed blocks and seek backwards
    // across content types; no result may depend on the previous file read.
    for (int pass = 0; pass < 2; ++pass) {
        for (size_t i = 0; i < entries.size(); ++i) {
            const auto& entry = entries[pass == 0 ? entries.size() - 1 - i : i];
            const Bytes want = load(root / "expected" / fs::path(entry.path).filename());
            check(want.size() == entry.size, "wrong expected fixture size: " + entry.path);
            check(disc.read(entry) == want, std::string(name) + ": bytes differ: " + entry.path);
        }
    }
}

void negative(const fs::path& root, const char* name, const WiiUDiscKeys& keys) {
    // Require that input exists: missing fixtures must not count as a parser
    // rejection. Failure at construction OR at content access is acceptable.
    check(fs::is_regular_file(root / name), std::string("missing negative fixture: ") + name);
    rejected([&] {
        WiiUDisc disc((root / name).string(), keys);
        for (const auto& entry : disc.entries())
            disc.read(entry);
    }, name);
}

void corrupt_content(const fs::path& root, const char* name, const char* target,
                     const WiiUDiscKeys& keys) {
    WiiUDisc disc((root / name).string(), keys); // Metadata must still be valid.
    const auto& entries = disc.entries();
    auto find = [&](const char* path) -> const WiiUDiscEntry& {
        const auto entry = std::find_if(entries.begin(), entries.end(),
                                       [&](const auto& e) { return e.path == path; });
        check(entry != entries.end(), std::string(name) + ": missing " + path);
        return *entry;
    };
    for (int attempt = 0; attempt < 2; ++attempt)
        rejected([&] { disc.read(find(target)); }, std::string(name) + ": corrupt block cached/accepted");
    check(disc.read(find("content/raw-head.bin")) == load(root / "expected/raw-head.bin"),
          std::string(name) + ": failed hashed read poisoned raw reader");
}

void run(const fs::path& root) {
    std::ifstream marker(root / "selftest.txt");
    std::string first;
    std::getline(marker, first);
    check(first == "PASS: scripts/wwhd/disc.py", "run the Python fixture generator/self-test first");

    const DiscKey disc = synthetic("SYNTHETIC-DISC!!");
    const DiscKey title = synthetic("SYNTHETIC-TITLE!");
    const DiscKey common = synthetic("SYNTHETIC-COMMON");
    const DiscKey wrong = synthetic("SYNTHETIC-WRONG!");
    check(wwhd::read_disc_keys((root / "SYNTHETIC-disc.key").string()) == std::vector<DiscKey>{disc},
          "binary disc key");
    check(wwhd::read_disc_keys((root / "SYNTHETIC-title.key").string()) == std::vector<DiscKey>{title},
          "binary title key");
    check(wwhd::read_disc_keys((root / "SYNTHETIC-common.key").string()) == std::vector<DiscKey>{common},
          "binary common key");
    check(wwhd::read_disc_keys((root / "SYNTHETIC-title-hex.key").string()) == std::vector<DiscKey>{title},
          "hex title key");
    const WiiUDiscKeys title_keys{{disc}, {title}, {}};
    const WiiUDiscKeys common_keys{{disc}, {}, {common}};
    const WiiUDiscKeys override_keys{{disc}, {title}, {wrong}};
    // A keys file holds every title's key: lists that start with keys for other
    // discs must still find this disc's own.
    const WiiUDiscKeys mixed_keys{{wrong, disc}, {wrong, title}, {}};
    const WiiUDiscKeys mixed_common{{wrong, disc}, {}, {wrong, common}};
    for (const char* image : {"valid.wud", "mapped.wux"}) {
        positive(root, image, title_keys);
        positive(root, image, common_keys);
        positive(root, image, override_keys);
        positive(root, image, mixed_keys);
        positive(root, image, mixed_common);
        for (const WiiUDiscKeys& keys : {
                 WiiUDiscKeys{{wrong}, {title}, {}},
                 WiiUDiscKeys{{disc}, {wrong}, {}},
                 WiiUDiscKeys{{disc}, {}, {wrong}},
                 WiiUDiscKeys{{disc}, {}, {}},
                 WiiUDiscKeys{{disc}, {wrong}, {common}},
                 WiiUDiscKeys{{wrong, wrong}, {title}, {}}}) {
            rejected([&] { WiiUDisc bad((root / image).string(), keys); },
                     std::string(image) + ": wrong/missing key or ignored title override");
        }
    }
    const char* malformed[] = {
        "bad-fst-magic.wud", "bad-fst-count.wud", "bad-cluster.wud",
        "bad-directory-backward.wud", "bad-directory-past-end.wud",
        "bad-directory-parent-end.wud", "bad-directory-depth.wud",
        "bad-name-offset.wud", "bad-name-unterminated.wud",
        "bad-path-traversal.wud", "bad-path-absolute.wud", "bad-path-backslash.wud",
        "bad-file-offset.wud", "bad-file-size.wud", "bad-cluster-offset.wud",
        "bad-fst-checksum.wud", "bad-identity.wud", "bad-toc.wud", "bad-partition.wud",
        "bad-short-header.wud", "bad-short-toc.wud", "bad-short-system.wud",
        "bad-short-fst.wud", "bad-short-content.wud", "bad-wux-short.wux", "bad-wux-map.wux",
    };
    for (const char* name : malformed)
        negative(root, name, title_keys);
    corrupt_content(root, "bad-hashed-data.wud", "content/hashed-cross.bin", title_keys);
    corrupt_content(root, "bad-hashed-h0.wud", "content/hashed-head.bin", title_keys);
    corrupt_content(root, "bad-hashed-last.wud", "content/nested/hashed-tail.bin", common_keys);
    // Bad-input attempts must not affect fresh instances or their caches.
    positive(root, "mapped.wux", common_keys);
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s fixtureDirectory\n", argv[0]);
        return 2;
    }
    try {
        run(fs::path(argv[1]));
        std::puts("Wii U generated fixtures: WUD/WUX, raw/CBC/hashed bytes, key paths and 29 malformed images PASS");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Wii U fixture test: %s\n", error.what());
        return 1;
    }
}
