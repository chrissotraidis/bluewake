// Generated fixtures only. Compile with wwhd/disc_image.cpp; no game data,
// keys, crypto, renderer or other importer sources are needed.
#include "wwhd/disc_image.h"
#include "wwhd/filesystem.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>

namespace {
using wwhd::Bytes;
using wwhd::DiscImage;
namespace fs = std::filesystem;
constexpr size_t kSector = 0x8000;
constexpr uint64_t kMaxDisc = uint64_t{64} * 1024 * 1024 * 1024;
constexpr uint64_t kU64Max = std::numeric_limits<uint64_t>::max();

static_assert(!std::is_copy_constructible_v<DiscImage> && !std::is_copy_assignable_v<DiscImage>);

void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}

template <typename Fn> void rejected(Fn action, const char* message) {
    try {
        action();
    } catch (const wwhd::FormatError&) {
        return;
    }
    throw std::runtime_error(message);
}

struct Fixtures {
    fs::path dir;

    explicit Fixtures(const fs::path& root) {
        fs::create_directories(root);
        dir = root / ("fixtures-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        check(fs::create_directory(dir), "fixture directory already exists");
    }

    ~Fixtures() {
        std::error_code error;
        fs::remove_all(dir, error);
    }

    std::string write(const char* name, const Bytes& data) const {
        const fs::path path = dir / name;
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!data.empty())
            file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        file.close();
        check(bool(file), "could not write generated fixture");
        return path.string();
    }
};

void put(Bytes& bytes, size_t at, uint64_t value, size_t count = 4) {
    for (size_t i = 0; i < count; ++i)
        bytes.at(at + i) = static_cast<uint8_t>(value >> (8 * i));
}

Bytes pattern(size_t sector) {
    Bytes result(kSector);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<uint8_t>(sector * 71 + i * 13 + (i >> 8));
    return result;
}

Bytes wux(uint64_t logical_size, const std::vector<uint32_t>& map, size_t physical_sectors) {
    const size_t base = ((32 + map.size() * 4 + kSector - 1) / kSector) * kSector;
    Bytes result(base);
    std::memcpy(result.data(), "WUX0", 4);
    put(result, 4, 0x1099D02E);
    put(result, 8, kSector);
    put(result, 16, logical_size, 8);
    for (size_t i = 0; i < map.size(); ++i)
        put(result, 32 + i * 4, map[i]);
    for (size_t i = 0; i < physical_sectors; ++i) {
        const Bytes data = pattern(i);
        result.insert(result.end(), data.begin(), data.end());
    }
    return result;
}

void bounds(DiscImage& image) {
    const uint64_t size = image.size();
    check(image.read(0, 0).empty(), "zero-length read at start");
    check(image.read(size, 0).empty(), "zero-length read at EOF");
    rejected([&] { image.read(size, 1); }, "accepted nonempty EOF read");
    rejected([&] { image.read(size + 1, 0); }, "accepted empty read past EOF");
    rejected([&] { image.read(kU64Max, 0); }, "accepted overflowing offset");
    rejected([&] { image.read(kU64Max, 2); }, "accepted wrapping offset plus size");
    rejected([&] { image.read(0, kU64Max); }, "accepted overflowing length");
    rejected([&] { image.read(0, uint64_t(wwhd::kMaxFile) + 1); }, "accepted read above kMaxFile");
    if (size != 0) {
        rejected([&] { image.read(size - 1, 2); }, "accepted read across EOF");
        rejected([&] { image.read(size - 1, kU64Max); }, "accepted wrapping length at EOF");
    }
    check(image.read(size, 0).empty(), "bounds errors poisoned reader");
}

void contents(DiscImage& image, const Bytes& expected) {
    check(image.size() == expected.size(), "wrong logical size");
    check(image.read(0, expected.size()) == expected, "whole-image bytes differ");
    const size_t offsets[] = {0, 1, kSector - 3, kSector - 1, kSector, kSector + 1,
                              2 * kSector - 1, 2 * kSector, expected.size()};
    const size_t counts[] = {0, 1, 3, 7, kSector - 1, kSector, kSector + 5};
    for (const size_t at : offsets) {
        for (const size_t count : counts) {
            if (at <= expected.size() && count <= expected.size() - at) {
                const auto begin = expected.begin() + static_cast<Bytes::difference_type>(at);
                const Bytes slice(begin, begin + static_cast<Bytes::difference_type>(count));
                check(image.read(at, count) == slice, "random-access or cross-sector bytes differ");
            }
        }
    }
    bounds(image);
    check(image.read(0, expected.size()) == expected, "read failed after bounds errors");
}

void raw(const Fixtures& fixtures) {
    Bytes expected = pattern(0);
    const Bytes next = pattern(1);
    expected.insert(expected.end(), next.begin(), next.end());
    expected.insert(expected.end(), {91, 27, 83, 6, 42});
    const std::string path = fixtures.write("raw.wud", expected);
    DiscImage image(path);
    contents(image, expected);
    check(image.read(expected.size() - 5, 5) == Bytes({91, 27, 83, 6, 42}), "raw trailing partial sector");
    for (size_t size = 0; size < 32; ++size) {
        const Bytes tiny(size, 42);
        DiscImage small(fixtures.write("tiny.raw", tiny));
        contents(small, tiny);
    }
    // The same open stream must report a short physical read if the input is
    // truncated after construction, and still allow subsequent valid reads.
    fs::resize_file(path, kSector);
    rejected([&] { image.read(kSector, 1); }, "accepted raw data truncated after open");
    check(image.read(0, 16) == Bytes(expected.begin(), expected.begin() + 16), "failed read poisoned stream");
}

void mapped(const Fixtures& fixtures) {
    // Physical sector 2 is unused; sector 3 is reused and read out of order.
    const std::vector<uint32_t> map{3, 1, 3, 0};
    Bytes expected;
    for (const uint32_t sector : map) {
        const Bytes data = pattern(sector);
        expected.insert(expected.end(), data.begin(), data.end());
    }
    expected.resize(3 * kSector + 19);
    Bytes data = wux(expected.size(), map, 4);
    put(data, 24, kU64Max, 8); // Reserved header bytes are ignored, as in Python.
    const std::string path = fixtures.write("mapped.wux", data);
    DiscImage image(path);
    contents(image, expected);
    const Bytes first = pattern(0);
    check(image.read(3 * kSector, 19) == Bytes(first.begin(), first.begin() + 19), "mapped trailing partial sector");
    check(image.read(2 * kSector, kSector) == image.read(0, kSector), "duplicated sector differs");
    fs::resize_file(path, 2 * kSector);
    rejected([&] { image.read(0, 1); }, "accepted mapped data truncated after open");
    check(image.read(3 * kSector, 19) == Bytes(first.begin(), first.begin() + 19), "mapped failed-read recovery");

    DiscImage tiny(fixtures.write("partial.wux", wux(1, {0}, 1)));
    contents(tiny, Bytes{first.front()});
}

void large_logical_image(const Fixtures& fixtures) {
    // Only an 8 MiB map and two physical sectors represent 64 GiB. This also
    // checks alignment when the map itself spans multiple sectors.
    std::vector<uint32_t> map(static_cast<size_t>(kMaxDisc / kSector), 0);
    const uint64_t high = uint64_t{1} << 32;
    map[static_cast<size_t>(high / kSector)] = 1;
    map.back() = 1;
    const std::string path = fixtures.write("large-logical.wux", wux(kMaxDisc, map, 2));
    DiscImage image(path);
    check(image.size() == kMaxDisc, "lost 64-bit logical size");
    const Bytes first = pattern(0), second = pattern(1);
    check(image.read(high, kSector) == second, "lost 64-bit read offset");
    check(image.read(high - 1, 2) == Bytes({first.back(), second.front()}), "crossing 4 GiB differs");
    check(image.read(kMaxDisc - kSector, kSector) == second, "last sector at 64 GiB differs");
    bounds(image);
    check(fs::file_size(path) < 9 * 1024 * 1024, "expanded mapped image on disk");
}

void malformed(const Fixtures& fixtures) {
    const Bytes valid = wux(kSector, {0}, 1);
    auto bad = [&](const Bytes& bytes, const char* reason) {
        const std::string path = fixtures.write("bad.wux", bytes);
        rejected([&] { DiscImage image(path); }, reason);
        check(fs::remove(path), "failed constructor retained fixture file");
    };
    rejected([&] { DiscImage image((fixtures.dir / "missing.wud").string()); }, "accepted missing file");
    for (size_t size = 3; size < 32; ++size)
        bad(Bytes(valid.data(), valid.data() + size), "accepted truncated WUX header");
    for (size_t at = 3; at < 8; ++at) {
        Bytes data = valid;
        data[at] ^= 1;
        bad(data, "accepted invalid WUX magic or version");
    }
    for (const uint32_t flags : {1u, 0x80000000u, 0xFFFFFFFFu}) {
        Bytes data = valid;
        put(data, 12, flags);
        bad(data, "accepted unsupported WUX flags");
    }
    for (const uint32_t size : {0u, 1u, 0x4000u, 0x8001u, 0x10000u, 0xFFFFFFFFu}) {
        Bytes data = valid;
        put(data, 8, size);
        bad(data, "accepted unsupported sector size");
    }
    for (const uint64_t size : {uint64_t{0}, kMaxDisc + 1, uint64_t{1} << 63, kU64Max}) {
        Bytes data = valid;
        put(data, 16, size, 8);
        bad(data, "accepted invalid or overflowing logical size");
    }
    for (size_t size = 32; size < 36; ++size)
        bad(Bytes(valid.data(), valid.data() + size), "accepted short sector map");
    Bytes data = wux(kSector + 1, {0, 0}, 1);
    data.resize(36);
    bad(data, "rounded down partial logical sector count");
    data = valid;
    put(data, 16, kMaxDisc, 8);
    bad(data, "accepted short map for maximum logical size");
    for (const size_t size : {size_t{36}, kSector - 1, kSector, 2 * kSector - 1})
        bad(Bytes(valid.data(), valid.data() + size), "accepted missing alignment or truncated payload");
    data = wux(1, {0}, 1);
    data.resize(kSector + 1);
    bad(data, "accepted incomplete physical sector for partial logical sector");
    for (const uint32_t sector : {1u, 0x00020000u, 0xFFFFFFFEu, 0xFFFFFFFFu}) {
        data = valid;
        put(data, 32, sector);
        bad(data, "accepted missing sector or overflowing physical offset");
    }
    data = wux(3 * kSector, {0, 1, 0}, 1);
    bad(data, "failed to validate an interior map entry");
}
} // namespace

int main(int argc, char** argv) {
    try {
        const Fixtures fixtures(argc > 1 ? fs::path(argv[1]) : fs::path("build/wwhd-disc-reader"));
        raw(fixtures);
        mapped(fixtures);
        large_logical_image(fixtures);
        malformed(fixtures);
        std::puts("WUD/WUX reader: raw, mapped, 64-bit bounds and malformed-input tests passed");
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
