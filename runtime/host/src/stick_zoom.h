#ifndef BLUEWAKE_STICK_ZOOM_H
#define BLUEWAKE_STICK_ZOOM_H

#include <math.h>
#include <stdbool.h>

typedef struct {
    bool down;
    bool zooming;
    bool block_camera;
} BwStickZoomGesture;

typedef enum {
    BW_STICK_ZOOM_IDLE,
    BW_STICK_ZOOM_ACTIVE,
    BW_STICK_ZOOM_TAP,
    BW_STICK_ZOOM_FINISHED,
} BwStickZoomAction;

typedef enum {
    BW_STICK_TAP_IDLE,
    BW_STICK_TAP_HOLD,
    BW_STICK_TAP_ACCEPTED,
    BW_STICK_TAP_TIMED_OUT,
} BwStickTapAction;

// Distinguish a right-stick click from a click-and-tilt zoom gesture. A tap is
// reported on release, after the stick has had a chance to cross the dead zone.
// Up is negative (closer) and down is positive (farther).
static inline BwStickZoomAction bw_stick_zoom_update(BwStickZoomGesture* gesture, bool available, bool down,
                                                      double horizontal, double vertical, double dead_zone,
                                                      double* axis) {
    *axis = 0.0;
    if (!available) {
        gesture->down = false;
        gesture->zooming = false;
        gesture->block_camera = false;
        return BW_STICK_ZOOM_IDLE;
    }

    // A released stick often springs through a small arc. Keep that residual
    // motion out of the camera until the stick has actually returned to centre
    // after a zoom gesture.
    if (!down && gesture->block_camera && fabs(horizontal) <= dead_zone && fabs(vertical) <= dead_zone)
        gesture->block_camera = false;

    if (down && !gesture->down)
        gesture->zooming = false;
    if (down && (fabs(horizontal) > dead_zone || fabs(vertical) > dead_zone))
        gesture->zooming = true;

    BwStickZoomAction action = BW_STICK_ZOOM_IDLE;
    if (down && gesture->zooming) {
        if (fabs(vertical) > dead_zone) {
            const double magnitude = (fabs(vertical) - dead_zone) / (1.0 - dead_zone);
            *axis = copysign(magnitude > 1.0 ? 1.0 : magnitude, vertical);
        }
        action = BW_STICK_ZOOM_ACTIVE;
    } else if (!down && gesture->down) {
        action = gesture->zooming ? BW_STICK_ZOOM_FINISHED : BW_STICK_ZOOM_TAP;
        if (gesture->zooming)
            gesture->block_camera = true;
        gesture->zooming = false;
    }
    gesture->down = down;
    return action;
}

static inline double bw_stick_zoom_advance(double scale, double axis, double speed, double seconds,
                                            double minimum, double maximum) {
    const double next = scale + axis * speed * seconds;
    return next < minimum ? minimum : next > maximum ? maximum : next;
}

// A C-stick-up pulse must span game frames; a single pad sample is too short
// to enter first person. Keep it held until the camera accepts it or the
// bounded window expires. Store retrace+1 so zero remains the idle sentinel.
static inline BwStickTapAction bw_stick_tap_update(unsigned long long* started, unsigned long long retrace,
                                                    bool start, bool accepted, unsigned long long limit) {
    if (start && !accepted && *started == 0)
        *started = retrace + 1u;
    if (*started == 0)
        return BW_STICK_TAP_IDLE;
    if (accepted) {
        *started = 0;
        return BW_STICK_TAP_ACCEPTED;
    }
    if (retrace + 1u - *started < limit)
        return BW_STICK_TAP_HOLD;
    *started = 0;
    return BW_STICK_TAP_TIMED_OUT;
}

#endif
