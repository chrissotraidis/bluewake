// Player settings applied to the renderer and to a physical controller:
// the render resolution, camera-stick inversion and face-button remapping.
// Stored in NSUserDefaults; the menu changes them and calls
// bluewake_settings_changed(); the frame tick applies them.
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Keys in NSUserDefaults.
#define BW_RENDER_SCALE_KEY "BlueWake.RenderScale"        // 0 native, 1-4 x 640x480; default 3
#define BW_ANISOTROPY_KEY "BlueWake.Anisotropy"           // 1 game default, 4/8/16 forced; default 1
#define BW_SMOOTH_MOTION_KEY "BlueWake.SmoothMotion"      // in-between frames: 0 off, 1 (60 FPS), 3 (120 FPS); default 0
#define BW_INVERT_CAMERA_X_KEY "BlueWake.InvertCameraX"   // bool
#define BW_INVERT_CAMERA_Y_KEY "BlueWake.InvertCameraY"   // bool
#define BW_BUTTON_MAP_KEY "BlueWake.ButtonMap"            // {"A": nativeButton, ...}
#define BW_MOD_WIDESCREEN_KEY "BlueWake.Mod.Widescreen"   // bool, applies at launch
#define BW_MOD_HD_TEXTURES_KEY "BlueWake.Mod.HDTextures"  // bool, applies at launch
#define BW_MOD_BETTERWW_KEY "BlueWake.Mod.BetterWW"       // bool, applies at launch
// Better Wind Waker's patched executable the composite's variants were built
// from (betterww 4501481, default settings, scripts/mods/make_betterww_iso.sh).
#define BW_BETTERWW_DOL_SHA1 "e884a349a28ca534e272cf4c17737db587245cdd"

// The GameCube buttons a controller can remap, and the controller buttons
// they can come from (SDL gamepad buttons, named by position).
enum { BW_REMAP_COUNT = 6 };
const char* bluewake_remap_name(int index);           // "A", "B", "X", "Y", "Z", "Start"
unsigned bluewake_remap_default_native(int index);    // SDL_GamepadButton
enum { BW_NATIVE_CHOICES = 10 };
unsigned bluewake_native_choice(int index);           // SDL_GamepadButton
const char* bluewake_native_choice_name(int index);   // "Bottom face (A / Cross)"...
unsigned bluewake_remap_current_native(int index);    // effective mapping from defaults
void bluewake_remap_set(int index, unsigned native);  // swaps with the button that had it
void bluewake_remap_reset(void);

typedef struct BWSettingsSnapshot {
    unsigned generation;
    int render_scale;
    int anisotropy;  // 0 when unset (the renderer keeps its starting value)
    bool invert_x, invert_y;
    unsigned native[BW_REMAP_COUNT];
    bool remapped;
    int smooth_motion;  // in-between frames per game frame, 0 when off
} BWSettingsSnapshot;
extern BWSettingsSnapshot g_bw_settings;

void bluewake_settings_changed(void);  // main thread: re-read defaults, apply next tick
void bluewake_settings_tick(void);     // main thread, once per presented frame

#ifdef __cplusplus
}
#endif
