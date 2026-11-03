#include "pad_wire.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

int main(void) {
    DolPadState pad = {0};
    DolPadState live = {0};
    DolPadState merged = {0};
    u32 data0 = 0u;
    u32 data1 = 0u;

    bluewake_pad_wire_encode(&pad, 0u, &data0, &data1);
    assert(data0 == 0x00008080u);
    assert(data1 == 0x80800000u);

    bluewake_pad_wire_encode(&pad, 0x0100u, &data0, &data1);
    assert(data0 == 0x01008080u);
    assert((u16)(data0 >> 16) == 0x0100u);

    pad.stick_x = -12;
    pad.stick_y = 24;
    pad.substick_x = 5;
    pad.substick_y = -7;
    pad.trigger_left = 0x30u;
    pad.trigger_right = 0x40u;
    bluewake_pad_wire_encode(&pad, 0x1234u, &data0, &data1);
    assert(data0 == 0x12347498u);
    // 0x1234 carries the digital R bit (0x0020) and not the L bit (0x0040). A
    // digital R click must reach the guest with full analog travel behind it,
    // because the guest's own pause-menu page switch reads mDoCPd_R_LOCK_BUTTON,
    // which mDoCPd_Read derives from mTriggerRight and not from the button bit.
    // The pad's own 0x40 is therefore overridden to full travel while the left
    // trigger, whose bit is clear, keeps the value the pad reported.
    assert(data1 == 0x857930FFu);
    // The same click with no analog value behind it encodes the same way, and an
    // analog value with no click keeps its own.
    pad.trigger_right = 0u;
    bluewake_pad_wire_encode(&pad, 0x0020u, &data0, &data1);
    assert((data1 & 0xFFu) == 0xFFu);
    bluewake_pad_wire_encode(&pad, 0x0000u, &data0, &data1);
    assert((data1 & 0xFFu) == 0x00u);
    bluewake_pad_wire_encode(&pad, 0x0040u, &data0, &data1);
    assert(((data1 >> 8) & 0xFFu) == 0xFFu);

    pad.button = 0x0100u;
    pad.stick_x = -40;
    pad.stick_y = 10;
    pad.trigger_left = 20u;
    live.button = 0x0200u;
    live.stick_x = 30;
    live.stick_y = 80;
    live.trigger_left = 90u;
    live.trigger_right = 70u;
    bluewake_pad_merge(&pad, &live, &merged);
    assert(merged.button == 0x0300u);
    assert(merged.stick_x == -40);
    assert(merged.stick_y == 80);
    assert(merged.trigger_left == 90u);
    assert(merged.trigger_right == 70u);

    puts("Retail SPEC2 PAD wire encoding contract test passed.");
    return 0;
}
