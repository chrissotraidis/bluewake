// Generated fixtures only. Link with the same wwhd sources as
// wwhd_native_import.cpp; no game data or renderer is needed.
#include "wwhd/imaging.h"
#include "wwhd/pack.h"

#include <zlib.h>

#include <cstdio>
#include <cstring>
#include <utility>

using wwhd::Bytes;

static void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}

static void put(Bytes& b, size_t at, uint32_t value, size_t n = 4) {
    for (size_t i = 0; i < n; ++i)
        b.at(at + i) = uint8_t(value >> (8 * (n - i - 1)));
}

static Bytes sarc(const std::vector<std::pair<std::string, Bytes>>& members) {
    const size_t sfnt = 32 + 16 * members.size();
    Bytes b(sfnt + 8);
    std::memcpy(b.data(), "SARC", 4);
    put(b, 4, 20, 2);
    put(b, 6, 0xFEFF, 2);
    std::memcpy(b.data() + 20, "SFAT", 4);
    put(b, 24, 12, 2);
    put(b, 26, uint32_t(members.size()), 2);
    std::memcpy(b.data() + sfnt, "SFNT", 4);
    put(b, sfnt + 4, 8, 2);
    for (size_t i = 0; i < members.size(); ++i) {
        put(b, 36 + i * 16, 0x01000000 | uint32_t((b.size() - sfnt - 8) / 4));
        const auto& name = members[i].first;
        b.insert(b.end(), name.begin(), name.end());
        b.push_back(0);
        while (b.size() % 4)
            b.push_back(0);
    }
    const size_t payload = b.size();
    put(b, 12, uint32_t(payload));
    for (size_t i = 0; i < members.size(); ++i) {
        put(b, 40 + i * 16, uint32_t(b.size() - payload));
        const auto& data = members[i].second;
        b.insert(b.end(), data.begin(), data.end());
        put(b, 44 + i * 16, uint32_t(b.size() - payload));
    }
    put(b, 8, uint32_t(b.size()));
    return b;
}

static Bytes yaz0(const Bytes& input) {
    Bytes b(16);
    std::memcpy(b.data(), "Yaz0", 4);
    put(b, 4, uint32_t(input.size()));
    for (size_t i = 0; i < input.size(); i += 8) {
        b.push_back(0xFF); // literal-only fixture
        b.insert(b.end(), input.begin() + i, input.begin() + std::min(i + 8, input.size()));
    }
    return b;
}

static void archives() {
    const Bytes payload = {1, 2, 3, 4, 5};
    const Bytes nested = sarc({{"plain", payload}, {"empty", {}}, {"compressed", yaz0(payload)}});
    const Bytes input = sarc({{"a", nested}, {"b", yaz0(nested)}});
    const Bytes unchanged = input;
    std::vector<std::pair<std::string, Bytes>> got;
    wwhd::leaves(input, "root", [&](const std::string& name, const Bytes& data) { got.emplace_back(name, data); });
    const std::vector<std::pair<std::string, Bytes>> expected = {
        {"root/a/plain", payload}, {"root/a/empty", {}}, {"root/a/compressed", payload},
        {"root/b/plain", payload}, {"root/b/empty", {}}, {"root/b/compressed", payload}};
    check(got == expected, "nested archive order, lifetime or contents changed");
    check(input == unchanged, "archive traversal changed its input");
    // Exercise RARC borrowing too (one root node, one file).
    Bytes rarc(128);
    std::memcpy(rarc.data(), "RARC", 4);
    put(rarc, 8, 32);
    put(rarc, 12, 96);
    put(rarc, 32, 1);
    put(rarc, 36, 32);
    put(rarc, 44, 48);
    put(rarc, 52, 68);
    put(rarc, 74, 1, 2);
    put(rarc, 84, 0x01000000);
    put(rarc, 92, uint32_t(payload.size()));
    std::memcpy(rarc.data() + 100, "file", 5);
    rarc.insert(rarc.end(), payload.begin(), payload.end());
    put(rarc, 4, uint32_t(rarc.size()));
    got.clear();
    wwhd::leaves(yaz0(sarc({{"arc", rarc}})), "root",
                 [&](const std::string& name, const Bytes& data) { got.emplace_back(name, data); });
    check(got == std::vector<std::pair<std::string, Bytes>>{{"root/arc/file", payload}}, "RARC leaf changed");
    for (const Bytes& bad : {sarc({{"../unsafe", payload}}), yaz0(Bytes{'S', 'A', 'R', 'C'})}) {
        bool rejected = false;
        try {
            wwhd::leaves(bad, "root", [](const std::string&, const Bytes&) {});
        } catch (const wwhd::FormatError&) {
            rejected = true;
        }
        check(rejected, "malformed archive accepted");
    }
    bool propagated = false;
    try {
        wwhd::leaves(input, "root", [](const std::string&, const Bytes&) { throw wwhd::FormatError("visitor"); });
    } catch (const wwhd::FormatError& e) {
        propagated = std::string(e.what()) == "visitor";
    }
    check(propagated, "visitor error was lost");
}

static void png() {
    // Noise crosses the compressor's output-buffer boundary; flat images
    // cover highly compressible data and narrow/odd scanlines.
    for (auto [w, h] : {std::pair{1, 1}, {1, 513}, {257, 129}, {512, 512}}) {
        for (bool noise : {false, true}) {
            wwhd::Image image(w, h);
            uint32_t state = 1234567;
            for (auto& byte : image.rgba) {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                byte = noise ? uint8_t(state) : 42;
            }
            const Bytes png = wwhd::encode_png(image);
            Bytes packed;
            const wwhd::Reader r(png);
            check(png.size() >= 8 && std::memcmp(png.data(), "\x89PNG\r\n\x1a\n", 8) == 0, "PNG signature");
            for (size_t at = 8; at < png.size();) {
                const size_t size = r.u32(at);
                const uint8_t* chunk = r.take(at + 4, size + 4);
                check(r.u32(at + 8 + size) == crc32(0, chunk, uInt(size + 4)), "PNG CRC");
                if (std::memcmp(chunk, "IDAT", 4) == 0)
                    packed.insert(packed.end(), chunk + 4, chunk + 4 + size);
                at += size + 12;
            }
            Bytes raw;
            for (int y = 0; y < h; ++y) {
                raw.push_back(0);
                raw.insert(raw.end(), image.at(0, y), image.at(0, y) + size_t(w) * 4);
            }
            uLongf size = compressBound(uLong(raw.size()));
            Bytes before(size);
            check(compress2(before.data(), &size, raw.data(), uLong(raw.size()), 6) == Z_OK, "reference compression");
            before.resize(size);
            check(packed == before, "streamed PNG differs from whole-image compression");
            Bytes decoded(raw.size());
            size = uLongf(decoded.size());
            check(uncompress(decoded.data(), &size, packed.data(), uLong(packed.size())) == Z_OK, "PNG decompression");
            check(size == raw.size() && decoded == raw, "PNG pixels changed");
        }
    }
}

int main() {
    try {
        archives();
        png();
        std::puts("WWHD native archive and streamed PNG fixtures passed");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
