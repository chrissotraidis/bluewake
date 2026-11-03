#include "pad_event_schedule.h"

#include <string.h>

void bluewake_pad_event_schedule_init(BluewakePadEventSchedule* schedule) {
    memset(schedule, 0, sizeof(*schedule));
}

bool bluewake_pad_event_schedule_configure(BluewakePadEventSchedule* schedule,
                                           u16 buttons, u64 length) {
    if (buttons == 0u || length == 0u)
        return false;
    schedule->configured = true;
    schedule->buttons = buttons;
    schedule->length = length;
    return true;
}

bool bluewake_pad_event_schedule_configure_latched(
    BluewakePadEventSchedule* schedule, u16 buttons) {
    if (buttons == 0u)
        return false;
    schedule->configured = true;
    schedule->latched = true;
    schedule->buttons = buttons;
    return true;
}

bool bluewake_pad_event_schedule_trigger(BluewakePadEventSchedule* schedule,
                                         u64 event_retrace) {
    if (!schedule->configured || schedule->triggered)
        return false;
    schedule->triggered = true;
    schedule->start_retrace = event_retrace + 1u;
    return true;
}

bool bluewake_pad_event_schedule_rearm(BluewakePadEventSchedule* schedule) {
    if (!schedule->configured || schedule->latched || !schedule->triggered)
        return false;
    schedule->triggered = false;
    schedule->released = false;
    schedule->start_retrace = 0u;
    schedule->release_retrace = 0u;
    return true;
}

bool bluewake_pad_event_schedule_release(BluewakePadEventSchedule* schedule,
                                         u64 event_retrace) {
    if (!schedule->latched || !schedule->triggered || schedule->released)
        return false;
    schedule->released = true;
    schedule->release_retrace = event_retrace + 1u;
    return true;
}

u16 bluewake_pad_event_schedule_sample(const BluewakePadEventSchedule* schedule,
                                       u64 retrace, u16 fallback) {
    if (!schedule->triggered)
        return fallback;
    if (schedule->latched) {
        if (schedule->released && retrace >= schedule->release_retrace)
            return fallback;
        const bool active = retrace >= schedule->start_retrace &&
                            (!schedule->released ||
                             retrace < schedule->release_retrace);
        return active ? schedule->buttons : fallback;
    }
    const u64 end = schedule->start_retrace + schedule->length;
    return retrace >= schedule->start_retrace && retrace < end
               ? schedule->buttons
               : fallback;
}

void bluewake_pad_axis_event_schedule_init(
    BluewakePadAxisEventSchedule* schedule) {
    memset(schedule, 0, sizeof(*schedule));
}

bool bluewake_pad_axis_event_schedule_configure(
    BluewakePadAxisEventSchedule* schedule, s8 value, u64 length) {
    if (value == 0 || length == 0u)
        return false;
    schedule->configured = true;
    schedule->value = value;
    schedule->length = length;
    return true;
}

bool bluewake_pad_axis_event_schedule_trigger(
    BluewakePadAxisEventSchedule* schedule, u64 event_retrace) {
    if (!schedule->configured || schedule->triggered)
        return false;
    schedule->triggered = true;
    schedule->start_retrace = event_retrace + 1u;
    return true;
}

// The axis twin of the button schedule's rearm: a one-shot pulse has to be
// rearmed before it can fire again, which is what a caller stepping a menu
// cursor needs on every step.
bool bluewake_pad_axis_event_schedule_rearm(
    BluewakePadAxisEventSchedule* schedule) {
    if (!schedule->configured || !schedule->triggered)
        return false;
    schedule->triggered = false;
    schedule->start_retrace = 0u;
    return true;
}

s8 bluewake_pad_axis_event_schedule_sample(
    const BluewakePadAxisEventSchedule* schedule, u64 retrace, s8 fallback) {
    if (!schedule->triggered)
        return fallback;
    const u64 end = schedule->start_retrace + schedule->length;
    return retrace >= schedule->start_retrace && retrace < end
               ? schedule->value
               : fallback;
}
