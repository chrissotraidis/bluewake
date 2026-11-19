#ifndef BLUEWAKE_LINUX_STEAM_H
#define BLUEWAKE_LINUX_STEAM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Steam's Game Mode: adding BlueWake to the Steam library so it launches from
// Gaming Mode without the player hunting the AppImage down in Steam's file
// chooser (#581). Everything here works headlessly, with no Steam and no Deck:
// the detection is three files and a variable, and the writer is
// shortcuts.vdf. The window (linux_setup.cpp) and --add-to-steam
// (linux_entry.c) are the two callers.

// 1 when Steam's Game Mode is available on this machine, 0 when it is not.
// That is SteamDeck=1 in the environment (Steam sets it for apps it launches;
// Bazzite and the other Game Mode images set it too), ID=steamos in
// /etc/os-release, or a Valve Jupiter/Galileo board by DMI. BLUEWAKE_STEAM=0
// turns it off everywhere, which is also the way to keep a test away from a
// real Steam.
int bw_steam_available(void);

// The one line that says why, for the window and the log.
const char* bw_steam_reason(void);

// What the Steam library holds under `name` for `exe`:
//   BW_STEAM_ABSENT   no entry with that name
//   BW_STEAM_CURRENT  present, and it points at this AppImage: nothing to do
//   BW_STEAM_STALE    present, it points at another path (the AppImage moved
//                     or was renamed): re-adding updates that entry in place
// Returns BW_STEAM_ABSENT when there is no shortcuts.vdf at all.
enum {
    BW_STEAM_ABSENT = 0,
    BW_STEAM_CURRENT = 1,
    BW_STEAM_STALE = 2,
};

int bw_steam_shortcut_state(const char* identity, const char* name);

// Add the shortcut, or update the entry already carrying this name.
//   BW_STEAM_ADDED      a new entry was written
//   BW_STEAM_UPDATED    the existing entry was rewritten (its Exe moved)
//   BW_STEAM_UNCHANGED  it was already there and already right
//   BW_STEAM_FAILED     nothing was written; `detail` says why
// `identity` is what marks *this* shortcut as the same one: the AppImage's own
// path, which is also `exe` unless the AppImage had to be wrapped (see `exe`),
// and it is stored in the entry's ShortcutPath so a move is still recognised.
// `launch_options` may be NULL or empty. Steam must not be running when the
// file is written (it rewrites the file with its own copy when it exits), so
// this refuses rather than silently losing the entry.
enum {
    BW_STEAM_FAILED = -1,
    BW_STEAM_UNCHANGED = 0,
    BW_STEAM_ADDED = 1,
    BW_STEAM_UPDATED = 2,
};

int bw_steam_add_shortcut(const char* exe, const char* identity, const char* name,
                          const char* launch_options, char* detail, size_t detail_size);

// The shortcuts.vdf that would be read or written: $BLUEWAKE_STEAM_VDF when it
// is set (the tests' synthetic file), else the single numeric account
// directory under ~/.steam/steam/userdata or ~/.local/share/Steam/userdata.
// Returns 1 with the path, 0 with `out` empty when there is no Steam userdata
// (or when it is ambiguous: two accounts, and guessing is not worth the entry
// landing in the wrong library).
int bw_steam_shortcuts_path(char* out, size_t size);

// 1 when ~/.steam/steam/steam.pid names a process that is still running.
int bw_steam_running(void);

#ifdef __cplusplus
}
#endif

#endif
