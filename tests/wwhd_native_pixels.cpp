// Development check for the built-in Wind Waker HD importer's pixels: for each
// line "hd-source<TAB>file<TAB>gc-format<TAB>image_sha256" (from a pack's
// import-report.json), rebuilds that texture's images from the HD folder and
// compares the SHA-256 of their RGBA bytes with the report's.
#include "wwhd/imaging.h"

#include <CommonCrypto/CommonDigest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

struct Want {
    std::string file;
    int gc_format;
    std::string sha;
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s HD-content-folder expected.tsv\n", argv[0]);
        return 2;
    }
    std::multimap<std::string, Want> wants;
    std::ifstream in(argv[2]);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string source, file, fmt, sha;
        std::getline(s, source, '\t');
        std::getline(s, file, '\t');
        std::getline(s, fmt, '\t');
        std::getline(s, sha, '\t');
        wants.insert({source, {file, std::stoi(fmt), sha}});
    }
    size_t same = 0, differ = 0, failed = 0, seen = 0;
    std::map<std::string, bool> reached;
    const std::string root = argv[1];
    const auto check = [&](const wwhd::HDTexture& t) {
        const auto range = wants.equal_range(t.source);
        if (range.first == range.second)
            return;
        reached[t.source] = true;
        seen++;
        try {
            std::vector<wwhd::Image> images = wwhd::hd_images(t);
            wwhd::complete_mips(images);
            const bool mask = t.source.find(".bflim") != std::string::npos || t.source.find(".bffnt") != std::string::npos;
            for (auto it = range.first; it != range.second; ++it) {
                const auto variant = wwhd::for_gc_format(images, it->second.gc_format, mask);
                CC_SHA256_CTX ctx;
                CC_SHA256_Init(&ctx);
                for (const auto& im : variant)
                    CC_SHA256_Update(&ctx, im.rgba.data(), CC_LONG(im.rgba.size()));
                unsigned char digest[32];
                CC_SHA256_Final(digest, &ctx);
                char hex[65];
                for (int i = 0; i < 32; i++)
                    std::snprintf(hex + i * 2, 3, "%02x", digest[i]);
                if (it->second.sha == hex) {
                    same++;
                } else {
                    differ++;
                    if (differ <= 10)
                        std::printf("DIFFER %s %s %dx%d fmt=0x%llx\n", it->second.file.c_str(), t.source.c_str(),
                                    t.width, t.height, (long long)t.format);
                }
            }
        } catch (const wwhd::FormatError& ex) {
            failed++;
            std::printf("FAILED %s: %s\n", t.source.c_str(), ex.what());
        }
    };
    for (const std::string& rel : wwhd::hd_files(root)) {
        try {
            const wwhd::Bytes archive = wwhd::read_file(root + "/" + rel);
            wwhd::leaves(archive, rel, [&](const std::string& path, const wwhd::Bytes& data) {
                try {
                    if (path.size() > 6 && path.compare(path.size() - 6, 6, ".bflim") == 0) {
                        if (wants.count(path))
                            check(wwhd::bflim(data, path));
                    } else if (data.size() >= 4 && std::memcmp(data.data(), "FRES", 4) == 0) {
                        // Only the wanted textures, as the importer asks for its matches' names.
                        std::set<std::string> names;
                        for (auto it = wants.lower_bound(path + "#"); it != wants.end() && it->first.rfind(path + "#", 0) == 0; ++it)
                            names.insert(wwhd::lower(it->first.substr(path.size() + 1)));
                        if (!names.empty())
                            wwhd::bfres(data, path, &names, check);
                    }
                } catch (const wwhd::FormatError& ex) {
                    std::printf("SKIPPED %s: %s\n", path.c_str(), ex.what());
                }
            });
        } catch (const wwhd::FormatError&) {
        }
    }
    std::printf("expected %zu, textures seen %zu: same %zu, differ %zu, failed %zu\n", wants.size(), seen, same, differ,
                failed);
    for (const auto& [source, want] : wants)
        if (!reached.count(source))
            std::printf("NOT REACHED %s\n", source.c_str());
    return differ || failed ? 1 : 0;
}
