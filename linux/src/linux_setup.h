#ifndef BLUEWAKE_LINUX_SETUP_H
#define BLUEWAKE_LINUX_SETUP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// The install folder the player chose here before, remembered beside the
// settings file (install.txt). linux_entry asks for it before it picks the
// default data folder, so a choice made once survives later launches that do not
// name one. Returns 1 and the path (no trailing separator) when there is one.
int bw_setup_remembered_install_dir(char* out, size_t size);

// Show the Linux first-run/configuration window. `install_dir` is the folder the
// game is installed into now and the starting value of the window's own field;
// the chosen folder (no trailing separator) is returned in `install_dir_out`.
// Returns 1 to continue into the game, 0 when the player closes or cancels it,
// and -1 after reporting an error. The selected disc is returned in `disc`; the
// texture-pack preference is written to the normal XDG settings file unless
// BLUEWAKE_SETTINGS=none.
int bw_linux_setup(const char* install_dir, char* install_dir_out, size_t install_dir_size,
                   char* disc, size_t disc_size);

#ifdef __cplusplus
}
#endif

#endif
