// BlueWake for Linux: the player's disc image. See linux_disc.c.
#pragma once

#include <stddef.h>

#include "disc_import.h"

#ifdef __cplusplus
extern "C" {
#endif
// Before the host starts (exe_dir and data_dir end in a separator): finds the
// game's files and sets BLUEWAKE_DISC, BLUEWAKE_DOL and BLUEWAKE_RELS_DIR.
// What the environment or --disc chose wins; a folder the builder made has
// them beside the app; otherwise the disc this player chose before, or, the
// first time and when it has gone, the one they choose now (checked and
// prepared once into data_dir/game). 0 when the game can start, 1 when the
// player chose no disc, -1 when the given disc cannot be used (said in a box).
// Returns whether the normal launcher has a usable disc source already: a
// prepared app folder, an explicitly selected disc, or a remembered disc that
// still exists. The AppImage uses this to decide whether first-run setup is
// needed.
int bw_disc_has_usable_source(const char* exe_dir, const char* data_dir);

// remember_explicit_disc saves a valid explicit selection as the future
// default; the setup UI uses it, while ordinary --disc overrides remain local
// to that launch. force_picker ignores the remembered selection and opens the
// standalone disc picker (--iso).
//
//   BLUEWAKE_DISC_CHOICE=FILE   testing: the file picker's answer, no dialogs
int bw_disc_setup(const char* exe_dir, const char* data_dir,
                  int remember_explicit_disc, int force_picker);

// Checks `disc` and, when the preparation remembered in data_dir is not
// already this disc's, unpacks its game files (main.dol and the 415 RELs) into
// a fresh folder there and remembers it. bw_disc_setup calls this, and so does
// the setup window, which runs it on a thread so it can show the importer's
// progress: the reports arrive through `progress` when it is not NULL, with
// `context` as given. *already_prepared (when given) is 1 when the remembered
// preparation was current and nothing had to be written. Returns 0 on success,
// otherwise -1 with a sentence for the player in `error`.
int bw_disc_prepare_into(const char* data_dir, const char* disc,
                         BlueWakeImportProgress progress, void* context,
                         int* already_prepared, char* error, size_t error_size);
#ifdef __cplusplus
}
#endif
