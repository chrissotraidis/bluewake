#ifndef BLUEWAKE_MOUSE_CAMERA_H
#define BLUEWAKE_MOUSE_CAMERA_H

#include "core/cpu.h"
#include "gxruntime/platform.h"

// Mouse camera (the Mac host): click the game window to hand it the mouse,
// then moving it turns the camera around Link, left and right and up and
// down, and its buttons press GameCube buttons (left click A unless BLUEWAKE_MOUSE_BUTTONS
// says otherwise); Esc, or leaving the window, gives the mouse back. The mouse turns the
// game's own camera (its view angles), so walls, the stick's direction and the
// camera's easing behind Link all carry on as they do.
//
// In first person (C-stick up) and when aiming an item (bow, hookshot,
// boomerang, grappling hook, telescope, Picto Box) the mouse aims instead:
// it turns Link's own aim, one to one and within the game's limits, and the
// view follows it with no lag; the stick still aims as well.
//
// The wheel zooms: in third person it brings the follow camera in or out
// (0.5x to 2x its distance, kept until changed); in the telescope and the
// Picto Box it is their own 1x-9x zoom, a step a notch.
//
// A controller's right stick (unless BLUEWAKE_STICK_CAMERA=0; on the Mac by
// default, on iOS only when asked for) turns the camera the same way wherever
// the mouse would (the follow camera): a turn rate from its tilt, no easing,
// the view held where it leaves it, instead of the game's eased C-stick camera.
// Its click is first person (and back out). In first person and when aiming an
// item it aims, as the mouse does; in the telescope and the Picto Box the left
// stick's up and down (or the D-pad's) zoom. Z-targeting and cutscenes keep the
// game's C-stick. The optional controller zoom keeps a click as first person,
// but holding it while moving the right stick up or down changes the normal
// follow camera's distance.
//
//   BLUEWAKE_MOUSE_CAMERA=0              off
//   BLUEWAKE_MOUSE_SENSITIVITY=1.0       degrees per point of mouse travel, scaled
//   BLUEWAKE_MOUSE_INVERT_Y=1            moving the mouse forward looks down
//   BLUEWAKE_MOUSE_BUTTONS=A,-,B,-,-     what the left, middle, right and two side
//                                        buttons press (input_remap.h); left A by default
//   BLUEWAKE_MOUSE_TRACE=1               log the camera's angles (and the aim's)
//   BLUEWAKE_MOUSE_TEST=r:dx:dy:n[:wheel],...  testing: motion per retrace from r for n
//   BLUEWAKE_MOUSE_TEST_ITEM=0x27[@900]  testing: that item on X (see grant_test_item)
//   BLUEWAKE_STICK_CAMERA=0              the game's own right stick (C-stick) instead
//   BLUEWAKE_STICK_CAMERA_SPEED=360      its turn at full tilt, degrees a second
//   BLUEWAKE_STICK_AIM_SPEED=180         the same when aiming
//   BLUEWAKE_STICK_ZOOM=1                hold its click and move it up/down to zoom the follow camera
//   BLUEWAKE_STICK_ZOOM_SPEED=1.275      zoom speed scale (1.0: two distance units a second)
//   BLUEWAKE_STICK_CAMERA_INVERT_X=1, _INVERT_Y=1  reverse its left and right, up and down
//   BLUEWAKE_STICK_TEST=r:x:y:n[:click[:zoom[:left_y]]],...  testing: the sticks and D-pad from r for n

// Once, after the Aurora window exists.
void bluewake_mouse_camera_install(void);
// Once the guest is running (the camera's state is in its memory).
void bluewake_mouse_camera_attach(CPUState* cpu);
// Once per retrace.
void bluewake_mouse_camera_retrace(void);
// At every dispatch boundary (the chassis edge service): at bumpCheck's entry
// (the camera's wall, ground and water check) the mouse's and the stick's
// view angles are applied, so the game keeps the eye out of the ground; at
// camera_draw's entry the frame's camera is final, and the zoom is applied;
// at the player's update (daPy_Execute) the mouse's aim is. Three compares
// here, inline: a call at each of the 380,000 boundaries a retrace takes was
// 2 percent of the game thread.
#define BLUEWAKE_MOUSE_CAMERA_BUMP 0x80167F08u    // bumpCheck__9dCamera_cFUl
#define BLUEWAKE_MOUSE_CAMERA_DRAW 0x8017C350u    // camera_draw__FP20camera_process_class
#define BLUEWAKE_MOUSE_PLAYER_EXECUTE 0x80122D30u // daPy_Execute__FP9daPy_lk_c
void bluewake_mouse_camera_hook(CPUState* cpu, u32 address);
static inline bool bluewake_mouse_camera_observes(u32 address) {
    return address == BLUEWAKE_MOUSE_CAMERA_DRAW || address == BLUEWAKE_MOUSE_PLAYER_EXECUTE ||
           address == BLUEWAKE_MOUSE_CAMERA_BUMP;
}
static inline void bluewake_mouse_camera_dispatch(CPUState* cpu, u32 address) {
    if (__builtin_expect(bluewake_mouse_camera_observes(address), 0))
        bluewake_mouse_camera_hook(cpu, address);
}
// On every pad read, on channel 0's live state: left click is A; with the fast
// stick camera, the right stick kept from the game's camera and its click.
void bluewake_mouse_camera_pad(DolPadState* pad);
// Whether BLUEWAKE_STICK_TEST scripts the right stick (a headless run, which
// has no live pad, still passes it through bluewake_mouse_camera_pad).
bool bluewake_mouse_camera_scripted(void);
// Whether the mouse is the camera now (Esc gives it back), and giving it back.
bool bluewake_mouse_camera_captured(void);
// What each mouse button presses (BLUEWAKE_MOUSE_BUTTONS's form; NULL or "" is
// the default, left click A).
void bluewake_mouse_camera_buttons(const char* map);
void bluewake_mouse_camera_release(void);
// Reads BLUEWAKE_MOUSE_CAMERA, _SENSITIVITY and _INVERT_Y, and the
// BLUEWAKE_STICK_CAMERA settings, again (the options menu).
void bluewake_mouse_camera_reload(void);
// The Windows options overlay applies these directly and blocks gameplay input.
void bluewake_mouse_camera_configure(bool enabled, double sensitivity, bool invert_y);
void bluewake_mouse_camera_block(bool blocked);

#endif
