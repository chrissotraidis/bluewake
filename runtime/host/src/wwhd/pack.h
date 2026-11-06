// The built-in Wind Waker HD importer: makes a Dolphin-format texture pack from
// a Wind Waker HD content folder or disc image and the GameCube disc, as
// scripts/import_wwhd_textures.py does (its build()), for every BlueWake build.
#pragma once

#include "wwhd.h"
#include "cancel.h"

#include <atomic>
#include <functional>

namespace wwhd {

struct ImportProgress {
    size_t files_read = 0;   // HD resources read so far
    size_t files_total = 0;  // HD resources to read
    size_t replacements = 0; // replacement keys so far
    std::string stage;       // what is happening, for the menu
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
};

// Throws Cancelled before publication when the token is set, FormatError for
// invalid input, or another std::exception for I/O failures. Workers stop before
// staging cleanup. Once the completed folder is renamed into place, it is kept.
// The token must remain alive until this call returns (including worker joins).
ImportResult import_pack(const ImportOptions& options);

// A PNG of an RGBA image (8 bits per channel), deflated with zlib.
Bytes encode_png(const struct Image& image);

} // namespace wwhd
