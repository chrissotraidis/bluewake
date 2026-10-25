// The container readers and the GameCube index of the built-in Wind Waker HD
// importer (wwhd.h). Ported from scripts/wwhd/formats.py and
// scripts/import_wwhd_textures.py; the comments there explain the rules.
#include "wwhd.h"
#include "file_io.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>

namespace wwhd {

const uint8_t* Reader::take(size_t at, size_t size) const {
    if (at > size_ || size > size_ - at) {
        char text[96];
        std::snprintf(text, sizeof text, "truncated data at 0x%zx (%zu bytes)", at, size);
        throw FormatError(text);
    }
    return data_ + at;
}

uint16_t Reader::u16(size_t at) const {
    const uint8_t* p = take(at, 2);
    return big_ ? static_cast<uint16_t>(p[0] << 8 | p[1]) : static_cast<uint16_t>(p[1] << 8 | p[0]);
}

uint32_t Reader::u32(size_t at) const {
    const uint8_t* p = take(at, 4);
    return big_ ? (uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3])
                : (uint32_t(p[3]) << 24 | uint32_t(p[2]) << 16 | uint32_t(p[1]) << 8 | p[0]);
}

size_t Reader::relative(size_t at) const {
    const int32_t value = i32(at);
    if (value == 0)
        return 0;
    const int64_t target = static_cast<int64_t>(at) + value;
    if (target < 0)
        throw FormatError("negative relative offset");
    return static_cast<size_t>(target);
}

std::string Reader::string(size_t at) const {
    take(at, 1);
    const size_t limit = std::min(at + 4096, size_);
    const void* end = std::memchr(data_ + at, 0, limit - at);
    if (end == nullptr)
        throw FormatError("unterminated string");
    return std::string(reinterpret_cast<const char*>(data_ + at), static_cast<const uint8_t*>(end) - (data_ + at));
}

std::string lower(std::string_view text) {
    std::string out(text);
    for (char& c : out)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return out;
}

std::string base_name(std::string_view path) {
    while (!path.empty() && path.back() == '/')
        path.remove_suffix(1);
    const size_t slash = path.rfind('/');
    return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1));
}

std::string stem(std::string_view name) {
    const std::string base = base_name(name);
    const size_t dot = base.rfind('.');
    if (dot != std::string::npos && dot > 0 && dot < base.size() - 1)
        return base.substr(0, dot);
    return base;
}

static bool ends_with(std::string_view text, std::string_view suffix) {
    return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
}

static void safe_name(const std::string& name) {
    bool unsafe = name.empty() || name[0] == '/' || name.find('\\') != std::string::npos ||
                  name.find('\0') != std::string::npos;
    size_t start = 0;
    while (!unsafe && start <= name.size()) {
        const size_t slash = name.find('/', start);
        const std::string_view part(name.data() + start, (slash == std::string::npos ? name.size() : slash) - start);
        if (part == "..")
            unsafe = true;
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    if (unsafe)
        throw FormatError("unsafe archive member name");
}

Bytes yaz0(const uint8_t* data, size_t size) {
    check_cancel();
    if (size < 4 || std::memcmp(data, "Yaz0", 4) != 0)
        return Bytes(data, data + size);
    const Reader r(data, size);
    const uint32_t out_size = r.u32(4);
    if (out_size > kMaxFile)
        throw FormatError("Yaz0 decompression limit exceeded");
    Bytes out;
    out.reserve(out_size);
    size_t src = 16;
    int bits = 0;
    uint8_t code = 0;
    size_t next_check = 0;
    while (out.size() < out_size) {
        if (out.size() >= next_check) {
            WWHD_CHECKPOINT("yaz0-block");
            next_check = out.size() + kIoChunk;
        }
        if (bits == 0) {
            code = r.u8(src++);
            bits = 8;
        }
        if (code & 0x80) {
            out.push_back(r.u8(src++));
        } else {
            const uint8_t a = r.u8(src), b = r.u8(src + 1);
            src += 2;
            const size_t distance = ((a & 15u) << 8) + b + 1;
            size_t count = a >> 4;
            if (count == 0)
                count = r.u8(src++) + 18u;
            else
                count += 2;
            if (distance > out.size() || out.size() + count > out_size)
                throw FormatError("invalid Yaz0 back reference");
            const size_t from = out.size() - distance;
            for (size_t i = 0; i < count; ++i)
                out.push_back(out[from + i]);
        }
        bits--;
        code = static_cast<uint8_t>(code << 1);
    }
    return out;
}

using MemberVisitor = std::function<void(const std::string&, const uint8_t*, size_t)>;

static void sarc(const uint8_t* data, size_t size, const MemberVisitor& member) {
    Reader r(data, size);
    const uint8_t* bom = r.take(6, 2);
    if (bom[0] == 0xFF && bom[1] == 0xFE)
        r.set_big_endian(false);
    else if (!(bom[0] == 0xFE && bom[1] == 0xFF))
        throw FormatError("invalid SARC byte order");
    const size_t header = r.u16(4);
    const size_t total = r.u32(8), payload = r.u32(12);
    if (total != size || payload > total)
        throw FormatError("invalid SARC size");
    if (std::memcmp(r.take(header, 4), "SFAT", 4) != 0)
        throw FormatError("missing SFAT");
    const size_t count = r.u16(header + 6);
    const size_t nodes = header + r.u16(header + 4);
    const size_t sfnt = nodes + 16 * count;
    if (std::memcmp(r.take(sfnt, 4), "SFNT", 4) != 0)
        throw FormatError("missing SFNT");
    const size_t strings = sfnt + r.u16(sfnt + 4);
    for (size_t i = 0; i < count; ++i) {
        WWHD_CHECKPOINT("archive-member");
        const size_t node = nodes + i * 16;
        const uint32_t attr = r.u32(node + 4), start = r.u32(node + 8), end = r.u32(node + 12);
        if (!(attr >> 24))
            continue;
        const std::string name = r.string(strings + size_t(attr & 0xFFFFFF) * 4);
        safe_name(name);
        if (end < start)
            throw FormatError("invalid SARC member bounds");
        member(name, r.take(payload + start, end - start), end - start);
    }
}

static void rarc(const uint8_t* data, size_t size, const MemberVisitor& member) {
    const Reader r(data, size);
    const size_t base = r.u32(8);
    if (base != 32 || r.u32(4) != size)
        throw FormatError("invalid RARC header");
    const size_t payload = base + r.u32(12);
    const size_t count = r.u32(base), nodes = base + r.u32(base + 4);
    const size_t entries = base + r.u32(base + 12);
    const size_t strings = base + r.u32(base + 20);
    std::set<size_t> ancestors;
    std::function<void(size_t, const std::string&)> walk = [&](size_t index, const std::string& prefix) {
        if (index >= count || ancestors.count(index) || ancestors.size() > 32)
            throw FormatError("invalid RARC directory graph");
        const size_t node = nodes + index * 16;
        const size_t n = r.u16(node + 10), first = r.u32(node + 12);
        for (size_t i = first; i < first + n; ++i) {
            WWHD_CHECKPOINT("archive-member");
            const size_t e = entries + i * 20;
            const uint32_t attr_name = r.u32(e + 4), offset = r.u32(e + 8), length = r.u32(e + 12);
            const std::string name = r.string(strings + (attr_name & 0xFFFFFF));
            if (name == "." || name == "..")
                continue;
            const std::string path = prefix + name;
            safe_name(path);
            if ((attr_name >> 24) & 2) {
                ancestors.insert(index);
                walk(offset, path + "/");
                ancestors.erase(index);
            } else if ((attr_name >> 24) & 1) {
                member(path, r.take(payload + offset, length), length);
            }
        }
    };
    walk(0, "");
}

void leaves(const uint8_t* data, size_t size, const std::string& path, const LeafVisitor& visit, int depth) {
    check_cancel();
    if (depth > 8 || size > kMaxFile)
        throw FormatError("archive nesting or size limit exceeded");
    // Borrow uncompressed containers from their parent while descending.
    // Copying each level keeps several whole archives alive at once.
    Bytes plain;
    if (size >= 4 && std::memcmp(data, "Yaz0", 4) == 0) {
        plain = yaz0(data, size);
        data = plain.data();
        size = plain.size();
    }
    const bool is_sarc = size >= 4 && std::memcmp(data, "SARC", 4) == 0;
    const bool is_rarc = size >= 4 && std::memcmp(data, "RARC", 4) == 0;
    if (!is_sarc && !is_rarc) {
        if (plain.empty() && size)
            plain.assign(data, data + size);
        visit(path, plain);
        return;
    }
    const MemberVisitor member = [&](const std::string& name, const uint8_t* bytes, size_t length) {
        leaves(bytes, length, path + "/" + name, visit, depth + 1);
    };
    if (is_sarc)
        sarc(data, size, member);
    else
        rarc(data, size, member);
}

namespace {
struct Block {
    int w, h, bytes;
};
bool block_of(int format, Block& b) {
    switch (format) {
    case 0: case 8: case 14: b = {8, 8, 32}; return true;
    case 1: case 2: case 9: b = {8, 4, 32}; return true;
    case 3: case 4: case 5: case 10: b = {4, 4, 32}; return true;
    case 6: b = {4, 4, 64}; return true;
    default: return false;
    }
}

std::string hex64(uint64_t value) {
    char text[17];
    std::snprintf(text, sizeof text, "%016llx", static_cast<unsigned long long>(value));
    return text;
}
} // namespace

std::string GCTexture::filename() const {
    check_cancel();
    if (!cached_name.empty())
        return cached_name;
    if (!name_error.empty())
        throw FormatError(name_error);
    std::string out = "tex1_" + std::to_string(width) + "x" + std::to_string(height) + "_" +
                      hex64(XXH64(pixels.data(), pixels.size(), 0));
    if (format == 8 || format == 9 || format == 10) {
        unsigned lo = 0xFFFF, hi = 0;
        const auto see = [&](unsigned v) {
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        };
        if (format == 8) {
            for (size_t i = 0; i < pixels.size(); ++i) {
                if (i % kIoChunk == 0)
                    check_cancel();
                const uint8_t b = pixels[i];
                see(b >> 4);
                see(b & 15);
            }
        } else if (format == 9) {
            for (size_t i = 0; i < pixels.size(); ++i) {
                if (i % kIoChunk == 0)
                    check_cancel();
                see(pixels[i]);
            }
        } else {
            for (size_t i = 0; i + 1 < pixels.size(); i += 2) {
                if (i % kIoChunk == 0)
                    check_cancel();
                see((unsigned(pixels[i]) << 8 | pixels[i + 1]) & 0x3FFF);
            }
        }
        if (pixels.empty() || 2 * (size_t(hi) + 1) > palette.size())
            throw FormatError("texture references missing palette entries");
        out += "_" + hex64(XXH64(palette.data() + lo * 2, (hi + 1 - lo) * 2, 0));
    }
    return out + "_" + std::to_string(format) + ".png";
}

GCTexture bti(const Reader& r, size_t at, const std::string& source, const std::string& name) {
    const uint8_t* h = r.take(at, 32);
    GCTexture t;
    t.source = source;
    t.name = name;
    t.format = h[0];
    t.palette_format = h[9];
    Block b;
    if (!block_of(t.format, b))
        throw FormatError("unsupported GameCube texture format " + std::to_string(t.format));
    t.width = r.u16(at + 2);
    t.height = r.u16(at + 4);
    if (t.width <= 0 || t.width > 4096 || t.height <= 0 || t.height > 4096)
        throw FormatError("invalid BTI dimensions");
    const size_t size = size_t((t.width + b.w - 1) / b.w) * ((t.height + b.h - 1) / b.h) * b.bytes;
    const uint8_t* pixels = r.take(at + r.u32(at + 28), size);
    t.pixels.assign(pixels, pixels + size);
    if (t.format == 8 || t.format == 9 || t.format == 10) {
        const size_t length = size_t(r.u16(at + 10)) * 2;
        const uint8_t* palette = r.take(at + r.u32(at + 12), length);
        t.palette.assign(palette, palette + length);
    }
    t.mipmaps = std::max<int>(h[24], 1);
    return t;
}

void gc_textures(const Bytes& data, const std::string& path, std::vector<GCTexture>& out) {
    if (ends_with(lower(path), ".bti")) {
        out.push_back(bti(Reader(data), 0, path, stem(path)));
        return;
    }
    if (data.size() < 7)
        return;
    const bool j3d = std::memcmp(data.data(), "J3D1", 4) == 0 || std::memcmp(data.data(), "J3D2", 4) == 0;
    const bool model = std::memcmp(data.data() + 4, "bmd", 3) == 0 || std::memcmp(data.data() + 4, "bdl", 3) == 0 ||
                       std::memcmp(data.data() + 4, "bmt", 3) == 0;
    if (!j3d || !model)
        return;
    const Reader r(data);
    size_t at = 32;
    const uint32_t count = r.u32(12);
    for (uint32_t i = 0; i < count; ++i) {
        check_cancel();
        const uint8_t* magic = r.take(at, 4);
        const size_t size = r.u32(at + 4);
        if (size < 8)
            throw FormatError("invalid J3D chunk size");
        const Reader c(r.take(at, size), size);
        if (std::memcmp(magic, "TEX1", 4) == 0) {
            const size_t n = c.u16(8), headers = c.u32(12), strings = c.u32(16);
            if (c.u16(strings) != n)
                throw FormatError("J3D texture name count differs");
            for (size_t k = 0; k < n; ++k) {
                check_cancel();
                const std::string name = c.string(strings + c.u16(strings + 6 + k * 4));
                out.push_back(bti(c, headers + k * 32, path + "#" + name, name));
            }
        }
        at += size;
    }
}

std::string resource(std::string_view path) {
    const size_t hash = path.find('#');
    if (hash != std::string_view::npos)
        path = path.substr(0, hash);
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= path.size()) {
        size_t slash = path.find('/', start);
        if (slash == std::string_view::npos)
            slash = path.size();
        const std::string_view part = path.substr(start, slash - start);
        if (!part.empty() && part != ".")
            parts.push_back(lower(part));
        start = slash + 1;
    }
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        const std::string& part = *it;
        if (!ends_with(part, ".szs"))
            continue;
        const std::string body = part.substr(0, part.size() - 4);
        const size_t underscore = body.rfind('_');
        if (underscore != std::string::npos && underscore > 0) {
            const std::string kind = body.substr(underscore + 1);
            bool room = kind.size() > 4 && kind.compare(0, 4, "room") == 0;
            for (size_t i = 4; room && i < kind.size(); ++i)
                room = std::isdigit(static_cast<unsigned char>(kind[i])) != 0;
            if (room || kind == "stage")
                return "stage|" + body.substr(0, underscore) + "|" + kind;
        }
    }
    const auto stage = std::find(parts.begin(), parts.end(), "stage");
    if (stage != parts.end() && stage + 1 != parts.end()) {
        const std::string first = stem(*(stage + 1));
        if (parts.end() - stage >= 3 && ends_with(*(stage + 2), ".arc"))
            return "stage|" + first + "|" + stem(*(stage + 2));
        const size_t underscore = first.rfind('_');
        if (underscore != std::string::npos)
            return "stage|" + first.substr(0, underscore) + "|" + first.substr(underscore + 1);
        return "stage|" + first;
    }
    std::string archive;
    for (const std::string& part : parts)
        if (ends_with(part, ".arc") || ends_with(part, ".szs"))
            archive = stem(part);
    if (!archive.empty())
        return "object|" + archive;
    return "object|" + (parts.empty() ? std::string() : stem(parts.back()));
}

std::vector<GCTexture> index_gc(const std::string& iso_path, std::vector<ImportError>& errors) {
    check_cancel();
    std::ifstream file(iso_path, std::ios::binary);
    if (!file)
        throw FormatError("cannot open the GameCube disc " + iso_path);
    Bytes header(0x500);
    read_exact(file, header.data(), header.size(), "File too small for GameCube disc header");
    const Reader h(header);
    if (h.u32(28) != 0xC2339F3Du)
        throw FormatError("Not a GameCube disc");
    if (std::memcmp(header.data(), "GZLE01", 6) != 0)
        throw FormatError("Unsupported disc; expected GZLE01. Only GZLE01 USA rev 0 is supported.");
    if (header[7] != 0)
        throw FormatError("only GZLE01 revision 0 is supported");
    const uint32_t fst_offset = h.u32(0x424), fst_size = h.u32(0x428);
    if (fst_size > 16u * 1024 * 1024)
        throw FormatError("invalid GameCube file table");
    Bytes fst(fst_size);
    file.seekg(fst_offset);
    read_exact(file, fst.data(), fst.size(), "truncated GameCube file table");
    const Reader f(fst);
    const size_t entries = f.u32(8);
    const size_t strings = entries * 12;

    struct Entry {
        std::string path;
        uint32_t offset, size;
    };
    std::vector<Entry> files;
    std::function<void(size_t, size_t, const std::string&)> walk = [&](size_t i, size_t end, const std::string& prefix) {
        while (i < end) {
            WWHD_CHECKPOINT("gc-entry");
            const size_t e = i * 12;
            const uint8_t flags = f.u8(e);
            const uint32_t name_off = f.u32(e) & 0xFFFFFF;
            const uint32_t off = f.u32(e + 4), size = f.u32(e + 8);
            const std::string name = f.string(strings + name_off);
            if (flags == 0) {
                files.push_back({prefix + name, off, size});
                i++;
            } else if (flags == 1) {
                if (size <= i || size > end)
                    throw FormatError("invalid GameCube file table");
                walk(i + 1, size, prefix + name + "/");
                i = size;
            } else {
                i++;
            }
        }
    };
    walk(1, entries, "");

    std::vector<GCTexture> textures;
    for (const Entry& e : files) {
        WWHD_CHECKPOINT("gc-resource");
        const std::string l = lower(e.path);
        if (!(ends_with(l, ".arc") || ends_with(l, ".bti") || ends_with(l, ".bmd") || ends_with(l, ".bdl") ||
              ends_with(l, ".bmt")))
            continue;
        if (e.size > kMaxFile)
            throw FormatError("GameCube resource exceeds size limit");
        Bytes data(e.size);
        file.seekg(e.offset);
        read_exact(file, data.data(), data.size(), "truncated GameCube disc");
        try {
            leaves(data, e.path, [&](const std::string& name, const Bytes& leaf) {
                WWHD_CHECKPOINT("gc-leaf");
                const size_t first = textures.size();
                try {
                    gc_textures(leaf, name, textures);
                } catch (const FormatError& ex) {
                    errors.push_back({name, ex.what()});
                }
                for (size_t i = first; i < textures.size(); i++) {
                    check_cancel();
                    GCTexture& t = textures[i];
                    try {
                        t.cached_name = t.filename();
                    } catch (const FormatError& ex) {
                        t.name_error = ex.what();
                    }
                    const bool sprite = t.format <= 3 || t.format == 8 || t.format == 9;
                    if (!sprite)
                        Bytes().swap(t.pixels);
                }
            });
        } catch (const FormatError& ex) {
            errors.push_back({e.path, ex.what()});
        }
    }
    return textures;
}

} // namespace wwhd
