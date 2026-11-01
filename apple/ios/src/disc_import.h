// BlueWake first-run preparation of a user-owned GZLE01 disc image.
//
// Produces, from the disc alone, the two inputs the host reads besides the
// disc itself: main.dol and the rels/ directory, byte-identical to the macOS
// preparation (scripts/prepare.py plus the REL alignment quirk recorded in
// docs/archive/status/GATES.md). Pure C so it can be tested on the Mac.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*BlueWakeImportProgress)(void* context, double fraction,
                                       const char* stage);

// Checks the disc header: GameCube magic and the GZLE01 id. Returns 0 on
// success, otherwise writes a sentence for the user into error.
int bluewake_disc_check(const char* iso_path, char* error, size_t error_size);

// Whether out_dir has room for an import that needs `needed` bytes, given
// `available` free. Returns 0 when it does, -1 otherwise with a sentence naming
// the folder, the size needed and the size free. Split out from
// bluewake_disc_prepare so the wording is testable without a disc image.
int bluewake_disc_space_verdict(const char* out_dir, int64_t needed,
                               int64_t available, char* error,
                               size_t error_size);

// Checks the disc, verifies main.dol's SHA-1 against USA rev 0, and writes
// out_dir/main.dol and out_dir/rels/*.rel. Existing outputs are replaced only
// after everything was written. Returns 0 on success.
int bluewake_disc_prepare(const char* iso_path, const char* out_dir,
                          BlueWakeImportProgress progress, void* context,
                          char* error, size_t error_size);

#ifdef __cplusplus
}
#endif
