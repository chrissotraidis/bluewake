#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>

#include "gyro_aim.h"

static int near(double a, double b) { return fabs(a - b) < 1e-9; }

int main(void) {
    double yaw = 0.0, pitch = 0.0;
    // Turning the controller left (+Y) turns the view left (negative yaw).
    bw_gyro_turn(0.0, 1.0, 0.5, 1.0, &yaw, &pitch);
    assert(near(yaw, -28.64788975654116) && pitch == 0.0);
    // Tilting its far end up (+X) looks up (negative pitch).
    yaw = pitch = 0.0;
    bw_gyro_turn(1.0, 0.0, 0.5, 1.0, &yaw, &pitch);
    assert(yaw == 0.0 && near(pitch, -28.64788975654116));
    // Speed scales it; reports add up.
    yaw = pitch = 0.0;
    bw_gyro_turn(0.0, -0.5, 0.01, 2.0, &yaw, &pitch);
    bw_gyro_turn(0.0, -0.5, 0.01, 2.0, &yaw, &pitch);
    assert(near(yaw, 2.0 * 0.5 * 0.01 * 2.0 * 57.29577951308232));
    // A quarter turn at speed 1 is a quarter turn.
    yaw = pitch = 0.0;
    bw_gyro_turn(0.0, -3.141592653589793 / 2.0, 1.0, 1.0, &yaw, &pitch);
    assert(near(yaw, 90.0));
    return 0;
}
