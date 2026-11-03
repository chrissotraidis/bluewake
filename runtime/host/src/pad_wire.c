#include "pad_wire.h"

static s8 dominant_axis(s8 configured, s8 live) {
    const int configured_magnitude = configured < 0 ? -(int)configured
                                                    : (int)configured;
    const int live_magnitude = live < 0 ? -(int)live : (int)live;
    return live_magnitude > configured_magnitude ? live : configured;
}

void bluewake_pad_merge(const DolPadState* configured,
                        const DolPadState* live, DolPadState* merged_out) {
    *merged_out = *configured;
    merged_out->button |= live->button;
    merged_out->stick_x = dominant_axis(configured->stick_x, live->stick_x);
    merged_out->stick_y = dominant_axis(configured->stick_y, live->stick_y);
    merged_out->substick_x =
        dominant_axis(configured->substick_x, live->substick_x);
    merged_out->substick_y =
        dominant_axis(configured->substick_y, live->substick_y);
    if (live->trigger_left > merged_out->trigger_left)
        merged_out->trigger_left = live->trigger_left;
    if (live->trigger_right > merged_out->trigger_right)
        merged_out->trigger_right = live->trigger_right;
    if (live->analog_a > merged_out->analog_a)
        merged_out->analog_a = live->analog_a;
    if (live->analog_b > merged_out->analog_b)
        merged_out->analog_b = live->analog_b;
}

void bluewake_pad_wire_encode(const DolPadState* pad, u16 buttons,
                              u32* data0_out, u32* data1_out) {
    const u32 stick_x = (u32)(u8)((s16)pad->stick_x + 128);
    const u32 stick_y = (u32)(u8)((s16)pad->stick_y + 128);
    const u32 substick_x = (u32)(u8)((s16)pad->substick_x + 128);
    const u32 substick_y = (u32)(u8)((s16)pad->substick_y + 128);
    // The digital L and R clicks are switches at the end of an analog
    // trigger's travel, so on the hardware they always arrive with full travel
    // behind them - and the guest's own menu code depends on that. The pause
    // menu's page switch asks dMs_isPush_R_Button, which reads
    // mDoCPd_R_LOCK_BUTTON, which mDoCPd_Read derives from mTriggerRight and
    // not from the button bit; a digital-only R therefore reaches the guest as
    // a button and moves nothing (measured: twelve digital presses left dMc_c
    // null, one with analog R at full travel opened the collect page). A
    // keyboard has no analog axis, so the digital bit has to imply full travel
    // here; a pad that drives the axis itself is unaffected, because full
    // travel is the most it can report.
    u32 trigger_left = pad->trigger_left;
    u32 trigger_right = pad->trigger_right;
    if ((buttons & 0x0040u) != 0u)
        trigger_left = 0xFFu;
    if ((buttons & 0x0020u) != 0u)
        trigger_right = 0xFFu;
    *data0_out = ((u32)buttons << 16) | (stick_x << 8) | stick_y;
    *data1_out = (substick_x << 24) | (substick_y << 16) |
                 (trigger_left << 8) | trigger_right;
}
