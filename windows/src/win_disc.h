// BlueWake for Windows: the player's disc image. See win_disc.c.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Before the host starts (exe_dir and data_dir end in a separator): finds the
// game's files and sets BLUEWAKE_DISC, BLUEWAKE_DOL and BLUEWAKE_RELS_DIR.
// What the environment or --disc chose wins; a folder the builder made has
// them beside the app; otherwise the disc this player chose before, or, the
// first time and when it has gone, the one they choose now (checked and
// prepared once into data_dir\game). 0 when the game can start, 1 when the
// player chose no disc, -1 when the given disc cannot be used (said in a box).
//
//   BLUEWAKE_DISC_CHOICE=FILE   testing: the file picker's answer, no dialogs
int bw_disc_setup(const char* exe_dir, const char* data_dir);

#ifdef __cplusplus
}
#endif
