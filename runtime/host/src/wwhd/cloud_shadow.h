#pragma once

// Wind Waker HD's cloud shadow texture (ww_shadow_clouds) for the HD look's cloud shadows on HD's rule, as
// scripts/wwhd/cloud_shadow.py writes it: the sea stage's Stage.bfres (first_szs_permanent.pack's sea_Stage.szs),
// 4096 x 4096 BC4 with 13 mips, its untiled blocks as they are in a DDS (FourCC BC4U, every mip) at
// kHdCloudShadowFolder/kHdCloudShadowFile. Header-only, so no platform's build lists another file.

#include "wwhd.h"

#include <cstring>
#include <string_view>
#include <vector>

namespace wwhd {

inline constexpr const char* kHdCloudShadowFolder = "optional/hd-cloud-shadow";
inline constexpr const char* kHdCloudShadowFile = "ww_shadow_clouds.dds";
inline constexpr const char* kHdCloudShadowName = "ww_shadow_clouds";
inline constexpr int64_t kGx2Bc4Unorm = 0x34;

// The archive member that holds it: the sea stage's Stage.bfres.
inline bool cloud_shadow_source(std::string_view path) {
    constexpr std::string_view suffix = "/sea_Stage.szs/Stage.bfres";
    return path.size() >= suffix.size() && path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// A BC4 DDS of untiled mip levels (8-byte blocks, rows of blocks, as GX2 stores BC4).
inline Bytes dds_bc4(uint32_t width, uint32_t height, const std::vector<Bytes>& levels) {
    Bytes out;
    const auto put32 = [&](uint32_t v) {
        for (int i = 0; i < 4; ++i)
            out.push_back(uint8_t(v >> (8 * i)));
    };
    out.insert(out.end(), {'D', 'D', 'S', ' '});
    put32(124);
    put32(0x1 | 0x2 | 0x4 | 0x1000 | 0x20000 | 0x80000); // caps, height, width, pixel format, mip count, linear size
    put32(height);
    put32(width);
    put32(levels.empty() ? 0u : uint32_t(levels[0].size()));
    put32(0);
    put32(uint32_t(levels.size()));
    for (int i = 0; i < 11; ++i)
        put32(0);
    put32(32);
    put32(0x4); // FourCC
    out.insert(out.end(), {'B', 'C', '4', 'U'});
    for (int i = 0; i < 5; ++i)
        put32(0);
    put32(0x1000 | 0x8 | 0x400000); // texture, complex, mipmap
    for (int i = 0; i < 4; ++i)
        put32(0);
    for (const Bytes& level : levels)
        out.insert(out.end(), level.begin(), level.end());
    return out;
}

} // namespace wwhd
