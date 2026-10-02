// Render resolution and physical-controller settings. See controller_settings.h.
//
// Aurora keeps one mapping per connected SDL gamepad, reset to its defaults
// when the gamepad connects, so the settings are re-applied whenever the
// controller on port 0 changes as well as when the player changes them. The
// on-screen controls feed Aurora's virtual pad and are not affected.
#import <Foundation/Foundation.h>
#include <SDL3/SDL_gamepad.h>
#include <stdio.h>

#include "controller_settings.h"

static const char* const kRemapNames[BW_REMAP_COUNT] = {"A", "B", "X", "Y", "Z", "Start"};
// Aurora's standard defaults: face buttons by position, right shoulder Z.
static const unsigned kRemapDefault[BW_REMAP_COUNT] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_START};
static const unsigned kNative[BW_NATIVE_CHOICES] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK, SDL_GAMEPAD_BUTTON_START,
    SDL_GAMEPAD_BUTTON_BACK};
static const char* const kNativeNames[BW_NATIVE_CHOICES] = {
    "Bottom Face (A / Cross)", "Right Face (B / Circle)", "Left Face (X / Square)",
    "Top Face (Y / Triangle)", "Left Shoulder (LB / L1)", "Right Shoulder (RB / R1)",
    "Left Stick Click", "Right Stick Click", "Menu / Start", "View / Select"};

const char* bluewake_remap_name(int i) { return kRemapNames[i]; }

// NSProcessInfo thermal state for the session log: 0 nominal, 1 fair,
// 2 serious (iOS throttles), 3 critical.
extern "C" int bluewake_thermal_state(void) { return (int)NSProcessInfo.processInfo.thermalState; }
unsigned bluewake_remap_default_native(int i) { return kRemapDefault[i]; }
unsigned bluewake_native_choice(int i) { return kNative[i]; }
const char* bluewake_native_choice_name(int i) { return kNativeNames[i]; }

unsigned bluewake_remap_current_native(int i) {
    NSDictionary* map = [NSUserDefaults.standardUserDefaults dictionaryForKey:@BW_BUTTON_MAP_KEY];
    NSNumber* value = map[@(kRemapNames[i])];
    return value != nil ? value.unsignedIntValue : kRemapDefault[i];
}

void bluewake_remap_set(int index, unsigned native) {
    NSMutableDictionary* map = [NSMutableDictionary dictionary];
    for (int i = 0; i < BW_REMAP_COUNT; i++)
        map[@(kRemapNames[i])] = @(bluewake_remap_current_native(i));
    const unsigned previous = bluewake_remap_current_native(index);
    for (int i = 0; i < BW_REMAP_COUNT; i++)
        if (i != index && bluewake_remap_current_native(i) == native)
            map[@(kRemapNames[i])] = @(previous);  // swap, so no button is doubled
    map[@(kRemapNames[index])] = @(native);
    [NSUserDefaults.standardUserDefaults setObject:map forKey:@BW_BUTTON_MAP_KEY];
    bluewake_settings_changed();
}

void bluewake_remap_reset(void) {
    [NSUserDefaults.standardUserDefaults removeObjectForKey:@BW_BUTTON_MAP_KEY];
    bluewake_settings_changed();
}

// Snapshot read on the main thread when settings change; applied by
// controller_apply.cpp (which needs Aurora's pad header, whose BOOL clashes
// with Objective-C's).
BWSettingsSnapshot g_bw_settings = {1u, 3, 0, false, false, {0}, false, 0};

void bluewake_settings_changed(void) {
    NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
    g_bw_settings.render_scale = [defaults objectForKey:@BW_RENDER_SCALE_KEY] != nil
                                     ? (int)[defaults integerForKey:@BW_RENDER_SCALE_KEY] : 3;
    g_bw_settings.anisotropy = (int)[defaults integerForKey:@BW_ANISOTROPY_KEY];
    g_bw_settings.invert_x = [defaults boolForKey:@BW_INVERT_CAMERA_X_KEY];
    g_bw_settings.invert_y = [defaults boolForKey:@BW_INVERT_CAMERA_Y_KEY];
    g_bw_settings.remapped = [defaults dictionaryForKey:@BW_BUTTON_MAP_KEY] != nil;
    // Preserve the early fork's 60 FPS setting and BlueWake's existing 60/120 choice.
    const NSInteger smooth = [defaults objectForKey:@BW_SMOOTH_MOTION_KEY] != nil
        ? [defaults integerForKey:@BW_SMOOTH_MOTION_KEY]
        : ([defaults boolForKey:@"BlueWake.FrameInterpolation"] ? 1 : 0);
    g_bw_settings.smooth_motion = smooth == 1 || smooth == 3 ? (int)smooth : 0;
    for (int i = 0; i < BW_REMAP_COUNT; i++)
        g_bw_settings.native[i] = bluewake_remap_current_native(i);
    g_bw_settings.generation++;
}
