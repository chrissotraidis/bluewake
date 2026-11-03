#ifndef BLUEWAKE_SETTINGS_MENU_H
#define BLUEWAKE_SETTINGS_MENU_H

#include <stdbool.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// The options menu (the Mac host): Esc (once the mouse is given back), F1 or a
// controller's Back button opens it over the paused game. Its choices are the
// settings the host reads from the environment (BLUEWAKE_*, DOL_AURORA_*),
// saved in a settings file and applied at the next launch, and most of them
// at once.
//
//   BLUEWAKE_SETTINGS=path|none   the settings file (default: Mac ~/Library/
//                                 Application Support/BlueWake/settings.ini,
//                                 Linux ~/.config/BlueWake/settings.ini; the
//                                 keys are in settings_table.h); none: neither
//                                 read nor written

#if (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(_WIN32) || defined(__ANDROID__)
// The iOS, Windows and Android apps have their own menus.
static inline void bluewake_settings_load(void) {}
static inline void bluewake_settings_menu_install(void) {}
#ifdef _WIN32
bool bluewake_settings_menu_event(const void* sdl_event);
#else
static inline bool bluewake_settings_menu_event(const void* sdl_event) {
    (void)sdl_event;
    return false;
}
#endif
#else
// First thing in main: the saved settings go into the environment, over what
// the launch gave (the menu's choices win).
void bluewake_settings_load(void);
// Once Aurora is up: the menu's overlay and the pause.
void bluewake_settings_menu_install(void);
// Every SDL event: true when the menu took it (its keys, or anything while it
// is open), so nothing else acts on it.
bool bluewake_settings_menu_event(const void* sdl_event);
// True once the menu's Restart now quit the game: linux_entry.c starts it
// again, with the environment it began with, after a clean exit.
bool bluewake_settings_restart_requested(void);
// The folder with the saves and session logs, for the Config tab (linux_entry.c
// sets it; otherwise it's the settings file's folder).
void bluewake_settings_set_data_dir(const char* dir);
#endif

#ifdef __cplusplus
}
#endif

#endif
