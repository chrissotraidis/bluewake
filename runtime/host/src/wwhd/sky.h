#pragma once

// Wind Waker HD's sky and light palettes (each stage's Pale and Virt tables)
// for the HD look's BLUEWAKE_HD_SKY, as scripts/wwhd/sky.py makes them: the
// importer writes kHdSkyFolder/kHdSkyFile from the stages on both discs.
// Header-only, so no platform's build lists another file.

#include "wwhd.h"

#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace wwhd {

inline constexpr const char* kHdSkyFolder = "optional/hd-sky";
inline constexpr const char* kHdSkyFile = "stages.bin";

namespace sky_detail {

inline uint32_t be32(const Bytes& d, size_t at) {
    if (at + 4 > d.size())
        throw FormatError("truncated stage file");
    return uint32_t(d[at]) << 24 | uint32_t(d[at + 1]) << 16 | uint32_t(d[at + 2]) << 8 | d[at + 3];
}

inline void put32(Bytes& out, uint32_t v) {
    for (int s = 24; s >= 0; s -= 8)
        out.push_back(uint8_t(v >> s));
}

// A dzs's chunk table: four-letter tag -> (count, offset). Empty if `at` holds none.
inline std::map<std::string, std::pair<uint32_t, uint32_t>> chunks(const Bytes& d, size_t at = 0) {
    std::map<std::string, std::pair<uint32_t, uint32_t>> out;
    const uint32_t n = be32(d, at);
    for (uint32_t i = 0; i < n; i++) {
        const size_t e = at + 4 + 12 * size_t(i);
        out[std::string(reinterpret_cast<const char*>(&d.at(e)), 4)] = {be32(d, e + 4), be32(d, e + 8)};
    }
    return out;
}

// HD's linear colours to the GameCube's gamma, rounding halves to even as
// Python's round() does (scripts/wwhd/sky.py makes the same bytes).
inline uint8_t to_gamma(uint8_t v) {
    return uint8_t(std::nearbyint(std::pow(v / 255.0, 1 / 2.2) * 255.0));
}

inline void append(Bytes& out, const Bytes& from, size_t at, size_t size) {
    if (at + size > from.size())
        throw FormatError("truncated stage table");
    out.insert(out.end(), from.begin() + long(at), from.begin() + long(at + size));
}

inline void append_gamma(Bytes& out, const Bytes& from, size_t at, size_t size) {
    if (at + size > from.size())
        throw FormatError("truncated stage table");
    for (size_t i = 0; i < size; i++)
        out.push_back(to_gamma(from[at + i]));
}

} // namespace sky_detail

// Where HD's Stage.bfres keeps the stage's dzs (a chunk table naming Pale).
inline std::optional<size_t> find_dzs(const Bytes& b) {
    static const uint8_t pale[4] = {'P', 'a', 'l', 'e'};
    for (size_t p = 0; p + 4 <= b.size(); p++) {
        if (std::memcmp(&b[p], pale, 4) != 0)
            continue;
        const size_t lowest = p > 12 * 200 ? p - 12 * 200 : 0;
        for (size_t s = p >= 4 ? p - 4 : 0; s > lowest && s + 4 <= b.size(); s -= 4) {
            if ((p - s - 4) % 12)
                continue;
            const uint32_t n = sky_detail::be32(b, s);
            if (n == 0 || n >= 200 || s + 4 + 12 * size_t(n) <= p || s + 4 + 12 * size_t(n) > b.size())
                continue;
            bool names = true;
            for (uint32_t i = 0; i < n && names; i++)
                for (size_t c = 0; c < 4; c++)
                    names = names && b[s + 4 + 12 * i + c] >= 32 && b[s + 4 + 12 * i + c] < 127;
            if (!names)
                continue;
            const auto ch = sky_detail::chunks(b, s);
            bool near = ch.count("Pale") != 0;
            for (const auto& [tag, entry] : ch)
                near = near && entry.second < 0x400000u;
            if (near)
                return s;
        }
    }
    return std::nullopt;
}

// One stage's palettes from its GameCube dzs and HD's ('BWSK', version 2).
// Throws FormatError when they can't be paired.
inline Bytes convert_sky(const Bytes& gc, const Bytes& hd) {
    const auto gcc = sky_detail::chunks(gc), hdc = sky_detail::chunks(hd);
    if (!gcc.count("Pale") || !hdc.count("Pale"))
        throw FormatError("no Pale table");
    // A stage without a sky (an interior) has no Virt table.
    const auto none = std::pair<uint32_t, uint32_t>{0, 0};
    const auto [pn, po] = gcc.at("Pale");
    const auto [vn, vo] = gcc.count("Virt") ? gcc.at("Virt") : none;
    const auto [hpn, hpo] = hdc.at("Pale");
    const auto [hvn, hvo] = hdc.count("Virt") ? hdc.at("Virt") : none;
    if (pn != hpn || vn != hvn)
        throw FormatError("HD and GC tables differ in length");
    Bytes out = {'B', 'W', 'S', 'K'};
    sky_detail::put32(out, 2);
    sky_detail::put32(out, pn);
    sky_detail::put32(out, vn);
    sky_detail::append(out, gc, po, 0x2Cu * pn);
    for (uint32_t i = 0; i < pn; i++) {
        const size_t e = hpo + 0x6Cu * size_t(i);
        sky_detail::append_gamma(out, hd, e, 0x21);   // 11 colours
        sky_detail::append(out, hd, e + 0x21, 0x0B);  // Virt index, pad, fog start and end
    }
    sky_detail::append(out, gc, vo, 0x24u * vn);
    for (uint32_t i = 0; i < vn; i++) {
        const size_t e = hvo + 0x38u * size_t(i), g = vo + 0x24u * size_t(i);
        sky_detail::append(out, hd, e, 0x10);
        sky_detail::append_gamma(out, hd, e + 0x10, 3);  // cloud colour, the GameCube's alpha
        sky_detail::append(out, gc, g + 0x13, 1);
        sky_detail::append_gamma(out, hd, e + 0x14, 3);  // cloud centre colour, the GameCube's alpha
        sky_detail::append(out, gc, g + 0x17, 1);
        sky_detail::append_gamma(out, hd, e + 0x18, 9);  // sky, false sea, haze
        sky_detail::append(out, hd, e + 0x21, 3);
    }
    for (uint32_t i = 0; i < pn; i++)
        sky_detail::append(out, hd, hpo + 0x6Cu * size_t(i) + 0x30, 24);  // the first six multipliers
    return out;
}

// Every stage's ('BWSA', count, then name[16], length and data each), from
// {name: (GameCube dzs, HD dzs)}; the stages left out go in `skipped`.
inline Bytes pack_sky(const std::map<std::string, std::pair<Bytes, Bytes>>& stages,
               std::vector<std::pair<std::string, std::string>>& skipped) {
    Bytes entries;
    uint32_t count = 0;
    for (const auto& [name, pair] : stages) {
        if (name.size() > 15) {
            skipped.emplace_back(name, "name too long");
            continue;
        }
        Bytes one;
        try {
            one = convert_sky(pair.first, pair.second);
        } catch (const std::exception& e) {
            skipped.emplace_back(name, e.what());
            continue;
        }
        Bytes padded(16, 0);
        std::memcpy(padded.data(), name.data(), name.size());
        entries.insert(entries.end(), padded.begin(), padded.end());
        sky_detail::put32(entries, uint32_t(one.size()));
        entries.insert(entries.end(), one.begin(), one.end());
        count++;
    }
    Bytes out = {'B', 'W', 'S', 'A'};
    sky_detail::put32(out, count);
    out.insert(out.end(), entries.begin(), entries.end());
    return out;
}

} // namespace wwhd
