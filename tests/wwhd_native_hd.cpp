// Development check for the built-in Wind Waker HD importer: every BFLIM and
// BFRES texture in a decrypted HD content folder, as "source<TAB>WxH<TAB>format
// <TAB>components<TAB>xxh64 of each level", to compare with scripts/wwhd.
#include "wwhd/hd.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s HD-content-folder\n", argv[0]);
        return 2;
    }
    const std::string root = argv[1];
    size_t textures = 0, errors = 0;
    for (const std::string& rel : wwhd::hd_files(root)) {
        try {
            const wwhd::Bytes archive = wwhd::read_file(root + "/" + rel);
            wwhd::leaves(archive, rel, [&](const std::string& path, const wwhd::Bytes& data) {
                const auto print = [&](const wwhd::HDTexture& t) {
                    std::printf("%s\t%dx%d\t%lld\t%d,%d,%d,%d", t.source.c_str(), t.width, t.height,
                                static_cast<long long>(t.format), t.components[0], t.components[1], t.components[2],
                                t.components[3]);
                    for (const auto& level : t.levels)
                        std::printf("\t%016llx", static_cast<unsigned long long>(XXH64(level.data(), level.size(), 0)));
                    std::printf("\n");
                    textures++;
                };
                try {
                    if (path.size() > 6 && path.compare(path.size() - 6, 6, ".bflim") == 0)
                        print(wwhd::bflim(data, path));
                    else if (data.size() >= 4 && std::memcmp(data.data(), "FRES", 4) == 0)
                        wwhd::bfres(data, path, nullptr, print);
                } catch (const wwhd::FormatError& ex) {
                    std::printf("%s\terror: %s\n", path.c_str(), ex.what());
                    errors++;
                }
            });
        } catch (const wwhd::FormatError& ex) {
            std::printf("%s\terror: %s\n", rel.c_str(), ex.what());
            errors++;
        }
    }
    std::fprintf(stderr, "%zu textures, %zu errors\n", textures, errors);
    return 0;
}
