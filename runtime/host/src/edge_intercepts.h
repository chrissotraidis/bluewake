#ifndef BLUEWAKE_EDGE_INTERCEPTS_H
#define BLUEWAKE_EDGE_INTERCEPTS_H

#include "core/cpu.h"
#include "edge_intercept_table.h"

#include <stdbool.h>

// Address-owned interception is only one part of a safe generated edge.
// Callback readiness, interrupt delivery, cycle budget, and exceptions remain
// separate predicates.
bool bluewake_edge_address_requires_host(u32 address, u32 module1_raw_base);
bool bluewake_edge_observation_requires_host(u32 canonical_address);

// How many times the guest has entered the pause menu's own execute
// (d_menu_window.cpp's dMs_Execute) and the collect screen create beneath it.
// Zero here with START held is the difference between "the menu refused" and
// "the menu never ran": the second cannot be fixed from the pad at all.
void bluewake_edge_set_menu_path_observation(bool enabled);
u64 bluewake_edge_menu_execute_calls(void);
u64 bluewake_edge_menu_collect_calls(void);

// The chassis calls this once per block boundary and almost every call is a
// miss, so it is defined here rather than beside the two predicates above. A
// miss is one hash probe and a return, and the census counts 391,432 boundaries
// per play retrace, which makes the call frame, the argument shuffle and the
// return about six of the twenty-four instructions a miss costs - instructions
// the enclosing function does not need to spend because it already has a frame
// and does not preserve anything across the probe. The two callees below stay
// out of line: a miss never reaches them.
#if defined(__clang__) || defined(__GNUC__)
#define BLUEWAKE_EDGE_INLINE static inline __attribute__((always_inline))
#else
#define BLUEWAKE_EDGE_INLINE static inline
#endif
BLUEWAKE_EDGE_INLINE bool
bluewake_edge_requires_host(u32 address, u32 canonical_address,
                            u32 module1_raw_base) {
    // Every address-space key in this file has the 0x40000000 mirror bit clear,
    // so canonical_address == address for all of them and one lookup on the
    // canonical form cannot miss a key the switches would match.
    if (module1_raw_base == 0u || address != module1_raw_base + 0xD4u) {
        if (!bluewake_edge_maybe_intercept(canonical_address))
            return false;
    }
    if (bluewake_edge_address_requires_host(address, module1_raw_base) ||
        bluewake_edge_observation_requires_host(canonical_address))
        return true;

    switch (canonical_address) {
    case 0x80303A50u:
    case 0x80240EE8u:
    case 0x80241178u:
    case 0x802411F8u:
        return true;
    default:
        return false;
    }
}

#endif
