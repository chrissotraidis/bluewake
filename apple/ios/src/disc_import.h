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

// Returns non-zero when the player has asked to stop the import. Called between
// the entries of the disc's file table and between the members of its RELS.arc,
// so a cancel lands within a few files instead of at the end of the import.
typedef int (*BlueWakeImportCancelled)(void* context);

// Checks the disc header: GameCube magic and the GZLE01 id. Returns 0 on
// success, otherwise writes a sentence for the user into error.
int bluewake_disc_check(const char* iso_path, char* error, size_t error_size);

// Checks the header as bluewake_disc_check does, and then that the image holds
// every byte its own file table names. A copy that ended early passes the header
// check — that reads 0x440 bytes — and then sends the game reading past the end
// of the file. Returns 0 on success, otherwise a sentence for the user.
int bluewake_disc_check_complete(const char* iso_path, char* error,
                                 size_t error_size);

// Whether out_dir has room for an import that needs `needed` bytes, given
// `available` free. Returns 0 when it does, -1 otherwise with a sentence naming
// the folder, the size needed and the size free. Split out from
// bluewake_disc_prepare so the wording is testable without a disc image.
int bluewake_disc_space_verdict(const char* out_dir, int64_t needed,
                               int64_t available, char* error,
                               size_t error_size);

// Checks the disc, verifies main.dol's SHA-1 against USA rev 0, and writes
// out_dir/main.dol and out_dir/rels/*.rel. Existing outputs are replaced only
// after everything was written. `cancelled` (optional) is asked between files:
// when it answers non-zero the import stops, its temporary files and any partial
// output are removed, and out_dir is left as it was.
//
// Returns 0 on success, 1 when the import was cancelled, and -1 on a failure
// with a sentence for the player in `error`.
int bluewake_disc_prepare(const char* iso_path, const char* out_dir,
                          BlueWakeImportProgress progress, void* context,
                          BlueWakeImportCancelled cancelled,
                          void* cancel_context, char* error, size_t error_size);

#ifdef __cplusplus
}
#endif
