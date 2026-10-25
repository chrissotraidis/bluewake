// The built-in Wind Waker HD texture importer: readers for the two games'
// texture containers, and the GameCube side of the match. A C++ port of
// scripts/wwhd (formats.py, and import_wwhd_textures.py's index and resource
// rules), checked against it texture by texture, so every build can make the
// pack the script makes. Readers never write archive members to disk.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace wwhd {

struct FormatError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

using Bytes = std::vector<uint8_t>;

constexpr size_t kMaxFile = 256u * 1024u * 1024u;

// Bounded big- or little-endian reads over a byte range.
class Reader {
  public:
    Reader(const uint8_t* data, size_t size, bool big_endian = true) : data_(data), size_(size), big_(big_endian) {}
    explicit Reader(const Bytes& bytes, bool big_endian = true) : Reader(bytes.data(), bytes.size(), big_endian) {}

    size_t size() const { return size_; }
    const uint8_t* data() const { return data_; }
    void set_big_endian(bool big) { big_ = big; }

    const uint8_t* take(size_t at, size_t size) const;
    uint8_t u8(size_t at) const { return *take(at, 1); }
    uint16_t u16(size_t at) const;
    uint32_t u32(size_t at) const;
    int32_t i32(size_t at) const { return static_cast<int32_t>(u32(at)); }
    // The absolute offset a self-relative 32-bit pointer points to (0 when null).
    size_t relative(size_t at) const;
    // A NUL-terminated name of at most 4096 bytes, as stored (Shift-JIS bytes).
    std::string string(size_t at) const;

  private:
    const uint8_t* data_;
    size_t size_;
    bool big_;
};

Bytes yaz0(const uint8_t* data, size_t size);

// Each file in a nest of Yaz0, SARC and RARC archives, as "path/member/...".
using LeafVisitor = std::function<void(const std::string& path, const Bytes& data)>;
void leaves(const uint8_t* data, size_t size, const std::string& path, const LeafVisitor& visit, int depth = 0);
inline void leaves(const Bytes& data, const std::string& path, const LeafVisitor& visit) {
    leaves(data.data(), data.size(), path, visit, 0);
}

struct GCTexture {
    std::string source;
    std::string name;
    int width = 0;
    int height = 0;
    int format = 0;
    Bytes pixels;
    Bytes palette;
    int palette_format = 0;
    int mipmaps = 1;
    // index_gc() works the name out once and keeps the pixels only for the
    // formats sprites are fitted to (I4, I8, IA4, IA8, C4, C8), to save memory.
    std::string cached_name;
    std::string name_error;

    // Dolphin's replacement name: tex1_WxH_<xxh64 of the data>[_<palette>]_<format>.png
    std::string filename() const;
};

GCTexture bti(const Reader& r, size_t at, const std::string& source, const std::string& name);
void gc_textures(const Bytes& data, const std::string& path, std::vector<GCTexture>& out);

// What a texture belongs to, for matching: ("stage", stage, room),
// ("stage", stage) or ("object", archive), lower case, joined by '|'.
std::string resource(std::string_view path);

std::string lower(std::string_view text);
// PurePosixPath(name).stem and .name for one '/'-separated path.
std::string stem(std::string_view name);
std::string base_name(std::string_view path);

struct ImportError {
    std::string source;
    std::string reason;
};

// Every texture on a GameCube GZLE01 revision 0 disc image (.iso/.gcm), in
// file-table order. Unreadable resources are added to the error list.
std::vector<GCTexture> index_gc(const std::string& iso_path, std::vector<ImportError>& errors);

} // namespace wwhd
