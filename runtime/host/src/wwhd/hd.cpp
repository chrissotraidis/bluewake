// The Wind Waker HD side of the built-in importer: the HD disc's files, and
// its BFRES (FTEX) and BFLIM textures, untiled. Ported from scripts/wwhd/formats.py
// and import_wwhd_textures.py's hd_files. BFRES v3/v4 FTEX layout follows
// AboodXD's BFRES-Tool, the BFLIM footer BFLIM-Tool (both GPL-3.0-or-later).
#include "hd.h"
#include "file_io.h"
#include "filesystem.h"

#include "addrlib.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace wwhd {
namespace {

bool bcn(int64_t format) {
    const int64_t base = format & 0xFF;
    return base >= 0x31 && base <= 0x35;
}

} // namespace

std::vector<std::string> hd_files(const std::string& folder) {
    check_cancel();
    namespace fs = std::filesystem;
    std::vector<std::vector<std::string>> found;
    std::error_code ec;
    const fs::path root(folder);
    for (fs::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        WWHD_CHECKPOINT("folder-entry");
        std::error_code type;
        if (!it->is_regular_file(type))
            continue;
        const std::string ext = lower(it->path().extension().string());
        if (ext != ".szs" && ext != ".sarc" && ext != ".pack" && ext != ".bfres")
            continue;
        std::vector<std::string> parts;
        for (const auto& part : it->path().lexically_relative(root))
            parts.push_back(part.generic_string());
        found.push_back(std::move(parts));
    }
    if (ec)
        throw FormatError("cannot read the Wind Waker HD folder: " + ec.message());
    // Python sorts the paths part by part (PurePosixPath ordering).
    std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
        check_cancel();
        return a < b;
    });
    std::vector<std::string> paths;
    for (const auto& parts : found) {
        check_cancel();
        std::string path;
        for (const auto& part : parts)
            path += (path.empty() ? "" : "/") + part;
        paths.push_back(path);
    }
    return paths;
}

Bytes read_file(const std::string& path) {
    check_cancel();
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw FormatError("cannot open " + path);
    const std::streamoff size = file.tellg();
    if (size < 0 || static_cast<uint64_t>(size) > kMaxFile)
        throw FormatError("HD resource exceeds size limit");
    Bytes data(static_cast<size_t>(size));
    file.seekg(0);
    read_exact(file, data.data(), data.size(), "cannot read HD resource");
    return data;
}

HDTexture bflim(const Bytes& data, const std::string& path) {
    const Reader r(data);
    if (data.size() < 40)
        throw FormatError("invalid Wii U BFLIM footer");
    const size_t footer = data.size() - 40;
    if (std::memcmp(r.take(footer, 4), "FLIM", 4) != 0 || r.u8(footer + 4) != 0xFE || r.u8(footer + 5) != 0xFF)
        throw FormatError("invalid Wii U BFLIM footer");
    if (r.u16(footer + 6) != 20 || r.u32(footer + 12) != data.size())
        throw FormatError("invalid BFLIM file size");
    const size_t at = footer + 20;
    const bool imag = std::memcmp(r.take(at, 4), "imag", 4) == 0;
    const uint32_t size = r.u32(at + 4);
    const int w = r.u16(at + 8), h = r.u16(at + 10);
    const int fmt = r.u8(at + 14), packed = r.u8(at + 15);
    const uint32_t image_size = r.u32(at + 16);
    struct Format {
        int fmt;
        int64_t surface;
        uint8_t components[4];
    };
    static const Format kFormats[] = {
        {0, 1, {0, 0, 0, 5}},       {1, 1, {5, 5, 5, 0}},       {3, 7, {0, 0, 0, 1}},
        {5, 8, {0, 1, 2, 5}},       {9, 0x1A, {0, 1, 2, 3}},    {20, 0x1A, {0, 1, 2, 3}},
        {12, 0x31, {0, 1, 2, 3}},   {21, 0x431, {0, 1, 2, 3}},  {13, 0x32, {0, 1, 2, 3}},
        {22, 0x432, {0, 1, 2, 3}},  {14, 0x33, {0, 1, 2, 3}},   {23, 0x433, {0, 1, 2, 3}},
        {15, 0x34, {0, 0, 0, 5}},   {16, 0x34, {5, 5, 5, 0}},   {17, 0x35, {0, 0, 0, 1}},
    };
    const Format* format = nullptr;
    for (const Format& f : kFormats)
        if (f.fmt == fmt)
            format = &f;
    if (!imag || size != 16 || format == nullptr)
        throw FormatError("unsupported BFLIM format " + std::to_string(fmt));
    if (w <= 0 || w > 8192 || h <= 0 || h > 8192 || image_size > footer)
        throw FormatError("invalid BFLIM surface bounds");
    const int64_t tile = packed & 31, swizzle = int64_t(packed >> 5) << 8;
    const addrlib::SurfaceInfo info = addrlib::getSurfaceInfo(format->surface, w, h, 1, 1, tile, 0, 0);
    const uint8_t* tiled = r.take(0, image_size);
    if (static_cast<int64_t>(image_size) < info.surfSize)
        throw FormatError("truncated BFLIM surface");
    const Bytes raw = addrlib::deswizzle(w, h, info.height, format->surface, info.tileMode, swizzle, info.pitch,
                                         info.bpp, tiled, image_size);
    const int block = bcn(format->surface) ? 4 : 1;
    const size_t length = size_t((w + block - 1) / block) * ((h + block - 1) / block) * info.bpp / 8;
    if (length > raw.size())
        throw FormatError("truncated data");
    HDTexture t;
    t.source = path;
    t.name = stem(path);
    t.width = w;
    t.height = h;
    t.format = format->surface;
    t.components.assign(format->components, format->components + 4);
    t.levels.emplace_back(raw.begin(), raw.begin() + length);
    return t;
}

void bfres(const Bytes& data, const std::string& path, const std::set<std::string>* names,
           const std::function<void(HDTexture&&)>& yield) {
    const Reader r(data);
    const uint8_t version = r.u8(4);
    if ((version != 3 && version != 4) || r.u8(8) != 0xFE || r.u8(9) != 0xFF)
        throw FormatError("only Wii U BFRES v3/v4 is supported");
    const size_t group = r.relative(0x24);
    if (!group)
        return;
    const size_t count = r.u32(group + 4), entries = group + 24;
    for (size_t i = 0; i < count; ++i) {
        check_cancel();
        const size_t e = entries + i * 16;
        const std::string name = r.string(r.relative(e + 8));
        const size_t tex = r.relative(e + 12);
        if (names != nullptr && !names->count(lower(name)))
            continue;
        if (std::memcmp(r.take(tex, 4), "FTEX", 4) != 0)
            throw FormatError("missing FTEX");
        uint32_t v[16];
        for (int k = 0; k < 16; ++k)
            v[k] = r.u32(tex + 4 + 4 * k);
        const int64_t dim = v[0], w = v[1], h = v[2], depth = v[3], aa = v[6], image_size = v[8], mip_size = v[10],
                      tile = v[12], swizzle = v[13];
        int64_t mips = v[4];
        const int64_t fmt = v[5];
        if (dim != 1 || depth != 1 || aa || w <= 0 || w > 8192 || h <= 0 || h > 8192)
            throw FormatError("only 2D, single-sample FTEX surfaces are supported");
        if (mips < 1 || mips > 14)
            throw FormatError("invalid FTEX mip count");
        if (version == 4)
            mips = 1;
        HDTexture t;
        t.source = path + "#" + name;
        t.name = name;
        t.width = static_cast<int>(w);
        t.height = static_cast<int>(h);
        t.format = fmt;
        const uint8_t* components = r.take(tex + 0x88, 4);
        t.components.assign(components, components + 4);
        const uint8_t* image = r.take(r.relative(tex + 0xB0), static_cast<size_t>(image_size));
        const uint8_t* mip_data = nullptr;
        if (mip_size)
            mip_data = r.take(r.relative(tex + 0xB4), static_cast<size_t>(mip_size));
        uint32_t offsets[13];
        for (int k = 0; k < 13; ++k)
            offsets[k] = r.u32(tex + 0x44 + 4 * k);
        const addrlib::SurfaceInfo base = addrlib::getSurfaceInfo(fmt, w, h, depth, dim, tile, aa, 0);
        const int bw = bcn(fmt) ? 4 : 1;
        const int64_t bpp = addrlib::surfaceGetBitsPerPixel(fmt);
        if (!bpp)
            throw FormatError("unknown GX2 surface format");
        for (int64_t level = 0; level < mips; ++level) {
            check_cancel();
            const addrlib::SurfaceInfo info = addrlib::getSurfaceInfo(fmt, w, h, depth, dim, tile, aa, level);
            const uint8_t* tiled;
            if (level) {
                const int64_t start = int64_t(offsets[level - 1]) - (level == 1 ? base.surfSize : 0);
                const Reader m(mip_data, static_cast<size_t>(mip_size));
                if (start < 0)
                    throw FormatError("truncated data");
                tiled = m.take(static_cast<size_t>(start), static_cast<size_t>(info.surfSize));
            } else {
                tiled = Reader(image, static_cast<size_t>(image_size)).take(0, static_cast<size_t>(info.surfSize));
            }
            const int64_t lw = std::max<int64_t>(1, w >> level), lh = std::max<int64_t>(1, h >> level);
            const size_t length = size_t((lw + bw - 1) / bw) * size_t((lh + bw - 1) / bw) * size_t(bpp) / 8;
            const Bytes raw = addrlib::deswizzle(lw, lh, info.height, fmt, info.tileMode, swizzle, info.pitch, info.bpp,
                                                 tiled, static_cast<size_t>(info.surfSize));
            if (length > raw.size())
                throw FormatError("truncated data");
            t.levels.emplace_back(raw.begin(), raw.begin() + length);
        }
        yield(std::move(t));
    }
}

} // namespace wwhd
