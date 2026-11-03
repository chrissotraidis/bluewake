#ifndef BLUEWAKE_LINUX_SETUP_H
#define BLUEWAKE_LINUX_SETUP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Show the Linux first-run/configuration window. Returns 1 to continue into the game,
// 0 when the player closes or cancels it, and -1 after reporting an error.
// The selected disc is returned in `disc`; the texture-pack preference is
// written to the normal XDG settings file unless BLUEWAKE_SETTINGS=none.
int bw_linux_setup(const char* data_dir, char* disc, size_t disc_size);

#ifdef __cplusplus
}
#endif

#endif
