// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BLUEWAKE_FACE_BUTTON_SWAP_H
#define BLUEWAKE_FACE_BUTTON_SWAP_H
#include <stdbool.h>
#include <stdint.h>
// Swap GameCube targets from the controller's own defaults; no SDL position assumptions.
static inline uint32_t bw_swap_face_buttons(uint32_t buttons, bool ab, bool xy) {
    if (ab && !!(buttons & 0x0100u) != !!(buttons & 0x0200u)) buttons ^= 0x0300u;
    if (xy && !!(buttons & 0x0400u) != !!(buttons & 0x0800u)) buttons ^= 0x0c00u;
    return buttons;
}
#endif
