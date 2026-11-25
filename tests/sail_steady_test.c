// SPDX-License-Identifier: GPL-3.0-or-later
// Swift Sail's steadier boat (runtime/host/src/sail_steady.h).
#include "sail_steady.h"

#include <stdio.h>

static int failures;

static void expect(int ok, const char* what) {
    if (!ok) {
        fprintf(stderr, "FAIL: %s\n", what);
        failures++;
    }
}

int main(void) {
    // Riding on the water, above it, or exactly at the lowest allowed: untouched.
    float y = 100.0f, sink = -3.0f;
    expect(sail_steady(&y, 100.0f, &sink) == 0 && y == 100.0f && sink == -3.0f, "on the water: untouched");
    y = 135.0f, sink = 4.0f;
    expect(sail_steady(&y, 100.0f, &sink) == 0 && y == 135.0f && sink == 4.0f, "above the water: untouched");
    y = 80.0f, sink = -2.0f;
    expect(sail_steady(&y, 100.0f, &sink) == 0 && y == 80.0f && sink == -2.0f, "at the lowest: untouched");

    // Deeper than SAIL_STEADY_DEPTH (the game allows 60): held there, and a sink stops pulling down.
    y = 45.0f, sink = -12.0f;
    expect(sail_steady(&y, 100.0f, &sink) == 1 && y == 80.0f && sink == 0.0f, "too deep: held, sink stopped");

    // Held there while already rising: the rise is kept.
    y = 60.0f, sink = 5.0f;
    expect(sail_steady(&y, 100.0f, &sink) == 1 && y == 80.0f && sink == 5.0f, "too deep and rising: rise kept");

    // Negative heights work the same.
    y = -90.0f, sink = -1.0f;
    expect(sail_steady(&y, -50.0f, &sink) == 1 && y == -70.0f && sink == 0.0f, "negative heights");

    if (failures == 0)
        puts("sail_steady_test: all passed");
    return failures != 0;
}
