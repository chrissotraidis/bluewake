#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#include "dpad_menus_map.h"

static void check(uint8_t open, uint8_t status, uint16_t button, int8_t x, int8_t y, bool changed,
                  uint16_t want_button, int8_t want_x, int8_t want_y) {
    uint16_t b = button;
    int8_t sx = x, sy = y;
    assert(bw_dpad_menus_map(open, status, &b, &sx, &sy) == changed);
    assert(b == want_button && sx == want_x && sy == want_y);
}

int main(void) {
    // No menu, or a map, the name entry or the save screen: untouched.
    check(0, 1, 0x0008, 0, 0, false, 0x0008, 0, 0);
    for (uint8_t status = 0; status < 8; status++)
        if (status != 1 && status != 2)
            check(1, status, 0x0002, 0, 0, false, 0x0002, 0, 0);
    // Items and Quest Status: each direction, the other buttons kept.
    check(1, 1, 0x0008 | 0x0100, 0, 0, true, 0x0100, 0, 100);
    check(1, 1, 0x0004, 0, 0, true, 0, 0, -100);
    check(1, 2, 0x0001, 3, -2, true, 0, -100, 0);
    check(1, 2, 0x0002, 0, 0, true, 0, 100, 0);
    check(1, 1, 0x0002 | 0x0008, 0, 0, true, 0, 71, 71);
    check(1, 1, 0x0001 | 0x0002, 0, 0, true, 0, 0, 0);
    // The stick held wins; the direction is still not passed on.
    check(1, 1, 0x0002, -60, 0, true, 0, -60, 0);
    // No direction: untouched.
    check(1, 1, 0x0100, 50, 0, false, 0x0100, 50, 0);
    return 0;
}
