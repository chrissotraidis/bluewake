#ifndef BLUEWAKE_JUMP_BUTTON_H
#define BLUEWAKE_JUMP_BUTTON_H

#include "core/cpu.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Jump button: Space, or a controller's left bumper, makes Link jump when he
// stands, walks or runs on the ground under the player's control. It is the game's own jump,
// the one Link makes running off a ledge (daPy_lk_c::procAutoJump_init), so
// its animation, voice, arc, landing, ledge grabs and glides are the game's.
// Anywhere else (swimming, climbing, hanging, crawling, carrying, on the boat,
// targeting or aiming, in the air, in menus, cutscenes and events) a press
// does nothing.
//
//   BLUEWAKE_JUMP_BUTTON=0          off
//   BLUEWAKE_JUMP_TRACE=1           log each press, what came of it, and the jump
//   BLUEWAKE_JUMP_TEST=r,r,...      press it at these retraces (testing without
//                                   a keyboard; turns the trace on)
//   BLUEWAKE_JUMP_TEST_TARGET=r:n   hold L (targeting) for n retraces from r, for
//                                   testing that a press then does nothing

// Once the guest is running.
void bluewake_jump_button_attach(CPUState* cpu);
// Every SDL event the Aurora window sees (the mouse camera's observer passes
// them on): Space presses the button. Nothing on iOS.
void bluewake_jump_button_event(const void* sdl_event);
// Once per retrace.
void bluewake_jump_button_retrace(void);
// Touch input is separate from the GameCube button bits.
void bluewake_jump_button_touch(bool down);
// Reads BLUEWAKE_JUMP_BUTTON again (the options menu).
void bluewake_jump_button_reload(void);

// At every dispatch boundary (the chassis edge service). While a press waits,
// Link's next proc call decides it: if it enters his standing, idling or
// moving proc and he can jump, that entry becomes a tail call of
// procAutoJump_init - cpu->pc is moved there and the turn ends (true), so the
// next turn runs the jump in that proc's place. Otherwise false, having
// changed nothing in the guest. Only a flag is read when no press waits.
extern bool bluewake_jump_button_armed;
bool bluewake_jump_button_enter(CPUState* cpu, u32 address);
static inline bool bluewake_jump_button_dispatch(CPUState* cpu, u32 address) {
    return __builtin_expect(bluewake_jump_button_armed, 0) && bluewake_jump_button_enter(cpu, address);
}

#ifdef __cplusplus
}
#endif

#endif
