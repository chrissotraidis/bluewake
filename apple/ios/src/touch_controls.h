// BlueWake iOS input and pause glue (touch_controls.cpp) and the UIKit shell
// entry (BWGameOverlay.mm).
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Touch button bits (SunPad's normalized layout).
enum {
    BLUEWAKE_TOUCH_DPAD_LEFT = 1 << 0,
    BLUEWAKE_TOUCH_DPAD_RIGHT = 1 << 1,
    BLUEWAKE_TOUCH_DPAD_DOWN = 1 << 2,
    BLUEWAKE_TOUCH_DPAD_UP = 1 << 3,
    BLUEWAKE_TOUCH_Z = 1 << 4,
    BLUEWAKE_TOUCH_R = 1 << 5,
    BLUEWAKE_TOUCH_L = 1 << 6,
    BLUEWAKE_TOUCH_A = 1 << 8,
    BLUEWAKE_TOUCH_B = 1 << 9,
    BLUEWAKE_TOUCH_X = 1 << 10,
    BLUEWAKE_TOUCH_Y = 1 << 11,
    BLUEWAKE_TOUCH_START = 1 << 12,
    BLUEWAKE_TOUCH_JUMP = 1 << 13,
    BLUEWAKE_TOUCH_SPRINT = 1 << 14,
};

// Why the guest is held. It runs only while the set is empty.
enum {
    BLUEWAKE_PAUSE_INACTIVE = 1 << 0,  // app resigning active / background
    BLUEWAKE_PAUSE_MENU = 1 << 1,      // three-dot menu open
    BLUEWAKE_PAUSE_SETTINGS = 1 << 2,  // touch settings panel open
    BLUEWAKE_PAUSE_LAYOUT = 1 << 3,    // layout editor
    BLUEWAKE_PAUSE_ALERT = 1 << 4,     // a modal alert
    BLUEWAKE_PAUSE_AUDIO = 1 << 5,     // audio session interrupted (a call, Siri)
};

// Sticks are -127..127 with +y up.
typedef struct BlueWakeTouchPad {
    uint16_t buttons;
    int8_t stick_x, stick_y;
    int8_t c_stick_x, c_stick_y;
} BlueWakeTouchPad;

// Registers the frame hook and the hold with the Aurora backend. Call before
// the host initializes Aurora.
void bluewake_touch_controls_install(void);

void bluewake_touch_publish(const BlueWakeTouchPad* pad);
void bluewake_touch_clear(void);
// The last second's presented frames per second, game speed in percent of
// full speed, and longest gap between presents (for the FPS display).
void bluewake_fps_read(float* shown, float* speed, float* worst_ms);
// Frames reaching the display a second, in-between frames included (equal to
// the shown count when Display > Smooth Motion is off).
float bluewake_fps_display(void);
void bluewake_pause_set(unsigned reason, bool on);
unsigned bluewake_pause_reasons(void);

// BWGameOverlay.mm: adds the touch controls and menu over SDL's game view.
void bluewake_shell_install_overlay(void);

#ifdef __cplusplus
}
#endif
