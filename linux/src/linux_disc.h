// BlueWake for Linux: the player's disc image. See linux_disc.c.
#pragma once
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
#ifdef __cplusplus
}
#endif
