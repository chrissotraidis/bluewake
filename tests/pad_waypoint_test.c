#include "pad_waypoint.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>

static float stick_world_angle(const BluewakePadWaypointOutput* output,
                               float camera_angle) {
    const float raw = atan2f((float)output->stick_x,
                             -(float)output->stick_y);
    return raw + (float)M_PI + camera_angle;
}

int main(void) {
    BluewakePadWaypointOutput output;
    assert(!bluewake_pad_world_angle_steer(0, 0, 0, 0, 1, 0, &output));
    assert(bluewake_pad_world_angle_steer(
        (float)M_PI, 0, -10, 0, 0, 100, &output));
    assert(fabsf(remainderf(stick_world_angle(&output, 0.0f) -
                                (float)M_PI,
                            2.0f * (float)M_PI)) < 0.02f);
    assert(!bluewake_pad_waypoint_steer(0, 0, 0, 0, 0, 1, 1, 1, 0,
                                        &output));
    assert(!bluewake_pad_waypoint_steer(0, 0, 0, 0, 0, 0, 1, 1, 100,
                                        &output));
    assert(bluewake_pad_waypoint_steer(0, 0, 0, -10, 0, 0, 10, 10, 100,
                                       &output));
    const float expected = atan2f(10.0f, 10.0f);
    const float actual = stick_world_angle(&output, 0.0f);
    assert(fabsf(remainderf(actual - expected, 2.0f * (float)M_PI)) < 0.02f);
    assert(fabsf(output.distance - sqrtf(200.0f)) < 0.01f);

    assert(bluewake_pad_waypoint_steer(2, 3, 10, 0, 0, 0, -4, 9, 80,
                                       &output));
    const float camera = atan2f(-10.0f, 0.0f);
    const float target = atan2f(-6.0f, 6.0f);
    assert(fabsf(remainderf(stick_world_angle(&output, camera) - target,
                            2.0f * (float)M_PI)) < 0.03f);

    BluewakePadRoute route;
    assert(!bluewake_pad_route_parse(NULL, 50.0f, &route));
    assert(!bluewake_pad_route_parse("1,2;", 50.0f, &route));
    assert(!bluewake_pad_route_parse("1,2;bad,3", 50.0f, &route));
    assert(!bluewake_pad_route_parse("1,2", 0.0f, &route));
    assert(bluewake_pad_route_parse("1.5,-2;-3,4.25;8,9", 2.0f,
                                    &route));
    assert(route.count == 3u && route.index == 0u);
    float target_x;
    float target_z;
    assert(bluewake_pad_route_update(&route, 0.0f, 0.0f, &target_x,
                                     &target_z));
    assert(target_x == 1.5f && target_z == -2.0f && route.index == 0u);
    assert(bluewake_pad_route_update(&route, 1.0f, -2.0f, &target_x,
                                     &target_z));
    assert(target_x == -3.0f && target_z == 4.25f && route.index == 1u);
    assert(bluewake_pad_route_update(&route, -3.0f, 4.0f, &target_x,
                                     &target_z));
    assert(target_x == 8.0f && target_z == 9.0f && route.index == 2u);
    assert(!bluewake_pad_route_update(&route, 8.0f, 9.0f, &target_x,
                                      &target_z));
    assert(route.index == route.count);
    puts("Camera-relative PAD waypoint contract test passed.");
    return 0;
}
