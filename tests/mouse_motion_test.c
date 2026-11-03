// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <SDL3/SDL.h>
#include <assert.h>
#include "mouse_motion.h"

static void push_motion(SDL_WindowID window, float x, float y) {
    SDL_Event event = {0};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = window;
    event.motion.xrel = x;
    event.motion.yrel = y;
    assert(SDL_PushEvent(&event));
}

int main(void) {
    assert(SDL_InitSubSystem(SDL_INIT_EVENTS));
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    double x = 2.0, y = -3.0;
    push_motion(10, 4.0f, -5.0f);
    assert(bluewake_take_camera_motion(false, true, false, 10, &x, &y) == 0);
    assert(bluewake_take_camera_motion(true, false, false, 10, &x, &y) == 0);
    assert(bluewake_take_camera_motion(true, true, true, 10, &x, &y) == 0);
    assert(bluewake_take_camera_motion(true, true, false, 0, &x, &y) == 0);
    assert(x == 2.0 && y == -3.0);
    assert(SDL_HasEvent(SDL_EVENT_MOUSE_MOTION));
    // More than one donor-sized batch, interspersed with another window and
    // menu/quit events. Only the captured game window may lose its deltas.
    SDL_Event key = {0}; key.type = SDL_EVENT_KEY_DOWN;
    key.key.scancode = SDL_SCANCODE_ESCAPE; assert(SDL_PushEvent(&key));
    push_motion(20, 1000.0f, 2000.0f);
    SDL_Event wheel = {0}; wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.wheel.windowID = 10; wheel.wheel.y = 1.0f; assert(SDL_PushEvent(&wheel));
    for (int i = 0; i < 80; ++i) push_motion(10, 1.0f, -2.0f);
    SDL_Event quit = {0}; quit.type = SDL_EVENT_QUIT; assert(SDL_PushEvent(&quit));
    assert(bluewake_take_camera_motion(true, true, false, 10, &x, &y) == 249.0);
    assert(x == 86.0 && y == -168.0);
    assert(bluewake_take_camera_motion(true, true, false, 10, &x, &y) == 0);
    assert(x == 86.0 && y == -168.0); // never counted twice
    SDL_Event remaining[8];
    int n = SDL_PeepEvents(remaining, 8, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
    assert(n == 4);
    assert(remaining[0].type == SDL_EVENT_KEY_DOWN);
    assert(remaining[1].type == SDL_EVENT_MOUSE_MOTION && remaining[1].motion.windowID == 20);
    assert(remaining[1].motion.xrel == 1000.0f && remaining[1].motion.yrel == 2000.0f);
    assert(remaining[2].type == SDL_EVENT_MOUSE_WHEEL && remaining[2].wheel.y == 1.0f);
    assert(remaining[3].type == SDL_EVENT_QUIT);
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
    return 0;
}
