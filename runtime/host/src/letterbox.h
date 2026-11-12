#ifndef BLUEWAKE_LETTERBOX_H
#define BLUEWAKE_LETTERBOX_H

#include "core/cpu.h"

#include <stdbool.h>

// Native cutscene framing (#597): in widescreen, cutscenes fill the screen
// instead of a strip between black bars. The game's bars are a scissor in
// camera_draw, mTrimHeight lines at the top and the bottom of the 480-line
// picture, eased in a quarter of the way each frame (dCamera_c::CalcTrimSize).
// With this on, camera_draw sees no trim, and its camera's vertical view
// narrows to the height the bars would have left, so the picture is the
// cutscene's own band zoomed to fill the screen: at 16:9 what 4:3 showed
// between the bars, wider screens a little more at the sides, as Wind Waker HD
// shows its cutscenes. Never narrower than 4:3's own width. At 4:3 (the view's
// own aspect) and for the telescope (trim size 3) the game's bars stay. Both
// are put back at the camera's next update, so the game's easing and its
// camera carry on as they do.
//
//   BLUEWAKE_NATIVE_CUTSCENE_FRAMING=1  on (off by default)
//   BLUEWAKE_LETTERBOX_TRACE=1          log each frame the bars are taken away

// Once the guest is running.
void bluewake_letterbox_attach(CPUState* cpu);
// Reads BLUEWAKE_NATIVE_CUTSCENE_FRAMING again (the options menu).
void bluewake_letterbox_reload(void);
// A save state was loaded: what was held to put back is dropped.
void bluewake_letterbox_state_loaded(void);
// A save state is about to be written: the game's values go back first.
void bluewake_letterbox_state_saving(CPUState* cpu);

// At every dispatch boundary (the chassis edge service): camera_draw's entry
// (r3 the camera process) takes the bars away for this frame, and
// camera_execute's entry puts the game's values back before the camera runs.
// Only a flag is read while it is off and nothing is held.
#define BLUEWAKE_LETTERBOX_CAMERA_EXECUTE 0x8017C29Cu // camera_execute__FP20camera_process_class
#define BLUEWAKE_LETTERBOX_CAMERA_DRAW 0x8017C350u    // camera_draw__FP20camera_process_class
extern bool bluewake_letterbox_armed;                 // on, or values held to put back
void bluewake_letterbox_hook(CPUState* cpu, u32 address);
static inline bool bluewake_letterbox_observes(u32 address) {
    return bluewake_letterbox_armed &&
           (address == BLUEWAKE_LETTERBOX_CAMERA_DRAW || address == BLUEWAKE_LETTERBOX_CAMERA_EXECUTE);
}
static inline void bluewake_letterbox_dispatch(CPUState* cpu, u32 address) {
    if (__builtin_expect(bluewake_letterbox_observes(address), 0))
        bluewake_letterbox_hook(cpu, address);
}

#endif
