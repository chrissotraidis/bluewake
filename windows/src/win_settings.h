// BlueWake for Windows: the settings (a settings file and the in-game menu),
// the desktop hotkeys and the window's placement. See win_settings.cpp.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// First thing in main: the environment as BlueWake was started, for Restart.
void bw_settings_capture_environment(void);
// Read %APPDATA%\BlueWake\settings.ini (data_dir ends in a separator).
void bw_settings_load(const char* data_dir);
// Before the host starts: the settings as the host's variables, where the
// command line did not already set one, and the live ones through Aurora.
void bw_settings_apply_launch(void);
// Before the host starts: the menu and hotkeys draw in Aurora's frame.
void bw_settings_install(void);
// Call only after the host has shut down workers and closed its card.
int bw_settings_relaunch(void);
// From the game thread's keyboard hook, for a key going down: 1 when BlueWake
// handled it (the game does not see it). alt: Alt is held.
int bw_settings_key(unsigned virtual_key, int alt);

#ifdef __cplusplus
}
#endif

