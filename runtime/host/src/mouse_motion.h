// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BLUEWAKE_MOUSE_MOTION_H
#define BLUEWAKE_MOUSE_MOTION_H

#include <SDL3/SDL_events.h>
#include <math.h>

typedef struct {
    SDL_WindowID window;
    double *x, *y;
    double magnitude;
} BlueWakeMouseMotion;

static bool SDLCALL bluewake_filter_camera_motion(void* user, SDL_Event* event) {
    BlueWakeMouseMotion* motion = (BlueWakeMouseMotion*)user;
    if (event->type != SDL_EVENT_MOUSE_MOTION || event->motion.windowID != motion->window)
        return true;
    *motion->x += event->motion.xrel;
    *motion->y += event->motion.yrel;
    motion->magnitude += fabs(event->motion.xrel) + fabs(event->motion.yrel);
    return false; // The camera consumed this delta; do not deliver it again.
}

static double bluewake_take_camera_motion(bool enabled, bool captured, bool blocked,
                                          SDL_WindowID window, double* x, double* y) {
    if (!enabled || !captured || blocked || window == 0)
        return 0.0;
    BlueWakeMouseMotion motion = {window, x, y, 0.0};
    SDL_PumpEvents();
    // Keep other windows' motion and all non-motion events in their original
    // order for Aurora/the settings menu to observe at the normal present.
    SDL_FilterEvents(bluewake_filter_camera_motion, &motion);
    return motion.magnitude;
}

#endif
