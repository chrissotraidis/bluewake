#include "pad_event_schedule.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

int main(void) {
    BluewakePadEventSchedule schedule;
    bluewake_pad_event_schedule_init(&schedule);
    assert(!bluewake_pad_event_schedule_configure(&schedule, 0u, 2u));
    assert(!bluewake_pad_event_schedule_configure(&schedule, 0x0100u, 0u));
    assert(bluewake_pad_event_schedule_configure(&schedule, 0x0100u, 2u));
    assert(bluewake_pad_event_schedule_sample(&schedule, 330u, 0u) == 0u);
    assert(bluewake_pad_event_schedule_trigger(&schedule, 330u));
    assert(!bluewake_pad_event_schedule_trigger(&schedule, 400u));
    assert(bluewake_pad_event_schedule_sample(&schedule, 330u, 0u) == 0u);
    assert(bluewake_pad_event_schedule_sample(&schedule, 331u, 0u) == 0x0100u);
    assert(bluewake_pad_event_schedule_sample(&schedule, 332u, 0u) == 0x0100u);
    assert(bluewake_pad_event_schedule_sample(&schedule, 333u, 0u) == 0u);

    BluewakePadEventSchedule confirmation;
    bluewake_pad_event_schedule_init(&confirmation);
    assert(bluewake_pad_event_schedule_configure(&confirmation, 0x0100u, 2u));
    assert(bluewake_pad_event_schedule_trigger(&confirmation, 336u));
    assert(bluewake_pad_event_schedule_sample(&confirmation, 336u, 0u) == 0u);
    assert(bluewake_pad_event_schedule_sample(&confirmation, 337u, 0u) == 0x0100u);
    assert(bluewake_pad_event_schedule_sample(&confirmation, 338u, 0u) == 0x0100u);
    assert(bluewake_pad_event_schedule_sample(&confirmation, 339u, 0u) == 0u);
    assert(bluewake_pad_event_schedule_rearm(&confirmation));
    assert(!bluewake_pad_event_schedule_rearm(&confirmation));
    assert(bluewake_pad_event_schedule_trigger(&confirmation, 400u));
    assert(bluewake_pad_event_schedule_sample(&confirmation, 401u, 0u) == 0x0100u);
    assert(bluewake_pad_event_schedule_sample(&confirmation, 403u, 0u) == 0u);

    BluewakePadEventSchedule latched;
    bluewake_pad_event_schedule_init(&latched);
    assert(!bluewake_pad_event_schedule_configure_latched(&latched, 0u));
    assert(bluewake_pad_event_schedule_configure_latched(&latched, 0x1000u));
    assert(!bluewake_pad_event_schedule_release(&latched, 700u));
    assert(bluewake_pad_event_schedule_trigger(&latched, 700u));
    assert(bluewake_pad_event_schedule_sample(&latched, 700u, 0u) == 0u);
    assert(bluewake_pad_event_schedule_sample(&latched, 701u, 0u) == 0x1000u);
    assert(bluewake_pad_event_schedule_sample(&latched, 900u, 0u) == 0x1000u);
    assert(bluewake_pad_event_schedule_release(&latched, 900u));
    assert(!bluewake_pad_event_schedule_rearm(&latched));
    assert(!bluewake_pad_event_schedule_release(&latched, 901u));
    assert(bluewake_pad_event_schedule_sample(&latched, 900u, 0u) == 0x1000u);
    assert(bluewake_pad_event_schedule_sample(&latched, 901u, 0u) == 0u);

    BluewakePadAxisEventSchedule axis;
    bluewake_pad_axis_event_schedule_init(&axis);
    assert(!bluewake_pad_axis_event_schedule_configure(&axis, 0, 2u));
    assert(!bluewake_pad_axis_event_schedule_configure(&axis, -100, 0u));
    assert(bluewake_pad_axis_event_schedule_configure(&axis, -100, 2u));
    assert(bluewake_pad_axis_event_schedule_sample(&axis, 500u, 7) == 7);
    assert(bluewake_pad_axis_event_schedule_trigger(&axis, 500u));
    assert(!bluewake_pad_axis_event_schedule_trigger(&axis, 600u));
    assert(bluewake_pad_axis_event_schedule_sample(&axis, 500u, 7) == 7);
    assert(bluewake_pad_axis_event_schedule_sample(&axis, 501u, 7) == -100);
    assert(bluewake_pad_axis_event_schedule_sample(&axis, 502u, 7) == -100);
    assert(bluewake_pad_axis_event_schedule_sample(&axis, 503u, 7) == 7);
    // A menu cursor steps with the same schedule over and over, so the axis
    // pulse has to rearm, and a rearmed pulse has to carry the deflection the
    // caller set on it for the new step - which is how the save route walks
    // the pause menu's item table without a second schedule per direction.
    assert(bluewake_pad_axis_event_schedule_rearm(&axis));
    assert(!bluewake_pad_axis_event_schedule_rearm(&axis));
    axis.value = 127;
    axis.length = 3u;
    assert(bluewake_pad_axis_event_schedule_trigger(&axis, 600u));
    assert(bluewake_pad_axis_event_schedule_sample(&axis, 601u, 7) == 127);
    assert(bluewake_pad_axis_event_schedule_sample(&axis, 603u, 7) == 127);
    assert(bluewake_pad_axis_event_schedule_sample(&axis, 604u, 7) == 7);
    puts("Event-synchronized PAD pulse contract test passed.");
    return 0;
}
