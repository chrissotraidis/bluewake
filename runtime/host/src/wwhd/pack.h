// The built-in Wind Waker HD importer: makes a Dolphin-format texture pack from
// a Wind Waker HD content folder or disc image and the GameCube disc, as
// scripts/import_wwhd_textures.py does (its build()), for every BlueWake build.
#pragma once

#include "wwhd.h"
#include "cancel.h"

#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace wwhd {

struct ImportProgress {
    size_t files_read = 0;   // HD resources read so far
    size_t files_total = 0;  // HD resources to read
    size_t replacements = 0; // replacement keys so far
    std::string stage;       // what is happening, for the menu
    // The HD look's optional parts so far: places with HD's sky and light,
    // models with HD's shading, and whether its cloud shadow texture was found.
    size_t sky_stages = 0, shaded_models = 0;
    bool cloud_shadow = false;
};

struct ImportOptions {
    std::string hd_folder; // decrypted content folder, or a USA WUD/WUX image
    // Each of these is a key file (one key, or a keys.txt with one per line),
    // or a key typed or pasted into the import form. An empty disc key file
    // defaults to game.key beside the image, and an empty common key file to
    // common.key beside it.
    std::string disc_key_file;
    std::string title_key_file;  // optional decrypted disc title key
    std::string common_key_file; // otherwise the common key decrypts the ticket
    std::string gc_disc;   // the GameCube GZLE01 rev 0 .iso/.gcm
    std::string output;    // a new folder for the pack
    std::function<void(const ImportProgress&)> progress;
    const std::atomic<bool>* cancel = nullptr;
    // SHA-256 of every image and file in import-report.json, as the script
    // writes them, to compare packs (slower; the app leaves it off).
    bool exact_report = false;
    // Threads writing PNGs: 0 for the machine's cores less one (at most 4).
    unsigned threads = 0;
};

struct ImportResult {
    size_t replacements = 0;
    size_t conflicts = 0;
    std::vector<ImportError> errors;
    // The pack's optional parts (import-report.json's "optional"), for the summary.
    size_t sky_stages = 0;    // optional/hd-sky: HD's sky and light, places
    size_t sky_clouds = 0;    // optional/hd-clouds: HD's sky clouds, textures
    bool cloud_shadow = false; // optional/hd-cloud-shadow: HD's cloud shadow texture
    size_t normal_stages = 0, normal_objects = 0; // optional/hd-normals (and hd-colours): places, object models
};

namespace import_summary_detail {
inline std::string count(size_t n, const char* one, const char* many) {
    return std::to_string(n) + " " + (n == 1 ? one : many);
}
inline std::string list(const std::vector<std::string>& items) {
    std::string out;
    for (size_t i = 0; i < items.size(); ++i)
        out += (i == 0 ? "" : i + 1 == items.size() ? " and " : ", ") + items[i];
    return out;
}
} // namespace import_summary_detail

// What an import made, for a person: the textures, then the optional parts
// the HD look uses (import-report.json's "optional").
inline std::string describe_import(const ImportResult& r) {
    using namespace import_summary_detail;
    std::vector<std::string> made;
    if (r.sky_stages)
        made.push_back("HD's sky and light for " + count(r.sky_stages, "place", "places"));
    if (r.sky_clouds)
        made.push_back("its sky clouds");
    if (r.cloud_shadow)
        made.push_back("its cloud shadows");
    if (r.normal_stages || r.normal_objects)
        made.push_back("its shading for the sun shadows (" + count(r.normal_stages, "place", "places") + ", " +
                       count(r.normal_objects, "object", "objects") + ")");
    return "Imported " + count(r.replacements, "texture", "textures") + " from Wind Waker HD" +
           (made.empty() ? std::string(".") : ", with " + list(made) + ".");
}

// The optional parts the discs did not give ("" when none is missing).
inline std::string describe_import_left_out(const ImportResult& r) {
    using namespace import_summary_detail;
    std::vector<std::string> missing;
    if (!r.sky_stages)
        missing.push_back("HD's sky and light");
    if (!r.sky_clouds)
        missing.push_back("its sky clouds");
    if (!r.cloud_shadow)
        missing.push_back("its cloud shadows");
    if (!r.normal_stages && !r.normal_objects)
        missing.push_back("its shading for the sun shadows");
    return missing.empty() ? std::string() : "Not found on these discs: " + list(missing) + ".";
}

// Throws Cancelled before publication when the token is set, FormatError for
// invalid input, or another std::exception for I/O failures. Workers stop before
// staging cleanup. Once the completed folder is renamed into place, it is kept.
// The token must remain alive until this call returns (including worker joins).
ImportResult import_pack(const ImportOptions& options);

// A PNG of an RGBA image (8 bits per channel), deflated with zlib.
Bytes encode_png(const struct Image& image);

} // namespace wwhd
