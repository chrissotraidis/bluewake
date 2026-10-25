// The Wind Waker HD side of the built-in importer (hd.cpp).
#pragma once

#include "wwhd.h"

#include <functional>
#include <set>

namespace wwhd {

struct HDTexture {
    std::string source;
    std::string name;
    int width = 0;
    int height = 0;
    int64_t format = 0;
    std::vector<uint8_t> components; // GX2 component selection, 4 entries
    std::vector<Bytes> levels;       // untiled mip levels, as stored (BCn blocks or pixels)
};

// The HD folder's .szs, .sarc, .pack and .bfres files, relative, in the
// importer's order.
std::vector<std::string> hd_files(const std::string& folder);
Bytes read_file(const std::string& path);

HDTexture bflim(const Bytes& data, const std::string& path);
// The FTEX textures of a BFRES, only those named in names (lower case) when given.
void bfres(const Bytes& data, const std::string& path, const std::set<std::string>* names,
           const std::function<void(HDTexture&&)>& yield);

} // namespace wwhd
