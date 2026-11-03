#ifndef BLUEWAKE_DPAD_MENUS_MAP_H
#define BLUEWAKE_DPAD_MENUS_MAP_H

#include <stdbool.h>
#include <stdint.h>

// GZLE01 revision 0, d_menu_window.cpp: dMenu_pause (dMenu_flag), non-zero
// while a menu is open, and menu_status (dMenu_getMenuStatus).
enum {
    kBwDpadMenusOpen = 0x803F7097u,
    kBwDpadMenusStatus = 0x803F7095u,
    kBwMenuStatusItem = 1u,    // MENU_STATUS_ITEM
    kBwMenuStatusCollect = 2u, // MENU_STATUS_COLLECT (Quest Status)
};

// The GameCube pad's D-pad bits.
enum {
    kBwPadLeft = 0x0001u,
    kBwPadRight = 0x0002u,
    kBwPadDown = 0x0004u,
    kBwPadUp = 0x0008u,
    kBwPadDpad = 0x000Fu,
};

// True when it changed the pad. A full push is 100 (patch 0159's scale), 71 on
// each axis for a diagonal, at the gate.
static inline bool bw_dpad_menus_map(uint8_t menu_open, uint8_t menu_status, uint16_t* button,
                                     int8_t* stick_x, int8_t* stick_y) {
    if (menu_open == 0u ||
        (menu_status != kBwMenuStatusItem && menu_status != kBwMenuStatusCollect))
        return false;
    const uint16_t dpad = (uint16_t)(*button & kBwPadDpad);
    if (dpad == 0u)
        return false;
    *button = (uint16_t)(*button & ~kBwPadDpad);
    // The stick wins while it is held; the direction is dropped either way.
    if (*stick_x > 20 || *stick_x < -20 || *stick_y > 20 || *stick_y < -20)
        return true;
    const int x = ((dpad & kBwPadRight) ? 1 : 0) - ((dpad & kBwPadLeft) ? 1 : 0);
    const int y = ((dpad & kBwPadUp) ? 1 : 0) - ((dpad & kBwPadDown) ? 1 : 0);
    const int full = (x != 0 && y != 0) ? 71 : 100;
    *stick_x = (int8_t)(x * full);
    *stick_y = (int8_t)(y * full);
    return true;
}

#endif
