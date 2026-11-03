#ifndef BLUEWAKE_PAD_EVENT_SCHEDULE_H
#define BLUEWAKE_PAD_EVENT_SCHEDULE_H

#include "core/cpu.h"

typedef struct BluewakePadEventSchedule {
    bool configured;
    bool triggered;
    bool latched;
    bool released;
    u64 start_retrace;
    u64 release_retrace;
    u64 length;
    u16 buttons;
} BluewakePadEventSchedule;

typedef struct BluewakePadAxisEventSchedule {
    bool configured;
    bool triggered;
    u64 start_retrace;
    u64 length;
    s8 value;
} BluewakePadAxisEventSchedule;

void bluewake_pad_event_schedule_init(BluewakePadEventSchedule* schedule);
bool bluewake_pad_event_schedule_configure(BluewakePadEventSchedule* schedule,
                                           u16 buttons, u64 length);
bool bluewake_pad_event_schedule_configure_latched(
    BluewakePadEventSchedule* schedule, u16 buttons);
bool bluewake_pad_event_schedule_trigger(BluewakePadEventSchedule* schedule,
                                         u64 event_retrace);
bool bluewake_pad_event_schedule_rearm(BluewakePadEventSchedule* schedule);
bool bluewake_pad_event_schedule_release(BluewakePadEventSchedule* schedule,
                                         u64 event_retrace);
u16 bluewake_pad_event_schedule_sample(const BluewakePadEventSchedule* schedule,
                                       u64 retrace, u16 fallback);
void bluewake_pad_axis_event_schedule_init(
    BluewakePadAxisEventSchedule* schedule);
bool bluewake_pad_axis_event_schedule_configure(
    BluewakePadAxisEventSchedule* schedule, s8 value, u64 length);
bool bluewake_pad_axis_event_schedule_trigger(
    BluewakePadAxisEventSchedule* schedule, u64 event_retrace);
bool bluewake_pad_axis_event_schedule_rearm(
    BluewakePadAxisEventSchedule* schedule);
s8 bluewake_pad_axis_event_schedule_sample(
    const BluewakePadAxisEventSchedule* schedule, u64 retrace, s8 fallback);

#endif
