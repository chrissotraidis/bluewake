// Development check for the built-in Wind Waker HD importer (runtime/host/src/wwhd):
// prints the GameCube index as "resource<TAB>name<TAB>filename" lines, to compare
// with the same lines from scripts/import_wwhd_textures.py.
#include "wwhd/wwhd.h"

#include <cstdio>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s GZLE01.iso\n", argv[0]);
        return 2;
    }
    std::vector<wwhd::ImportError> errors;
    try {
        const auto textures = wwhd::index_gc(argv[1], errors);
        for (const auto& t : textures) {
            std::string file;
            try {
                file = t.filename();
            } catch (const wwhd::FormatError& ex) {
                file = std::string("error: ") + ex.what();
            }
            std::printf("%s\t%s\t%s\n", wwhd::resource(t.source).c_str(), wwhd::lower(t.name).c_str(), file.c_str());
        }
        std::fprintf(stderr, "%zu textures, %zu errors\n", textures.size(), errors.size());
    } catch (const wwhd::FormatError& ex) {
        std::fprintf(stderr, "error: %s\n", ex.what());
        return 1;
    }
    return 0;
}
