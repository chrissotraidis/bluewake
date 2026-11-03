#ifndef BLUEWAKE_PAD_WAYPOINT_H
#define BLUEWAKE_PAD_WAYPOINT_H

#include <stddef.h>

#include "core/cpu.h"

typedef struct BluewakePadWaypointOutput {
    s8 stick_x;
    s8 stick_y;
    float distance;
} BluewakePadWaypointOutput;

#define BLUEWAKE_PAD_ROUTE_MAX_POINTS 64u

typedef struct BluewakePadRoutePoint {
    float x;
    float z;
} BluewakePadRoutePoint;

typedef struct BluewakePadRoute {
    BluewakePadRoutePoint points[BLUEWAKE_PAD_ROUTE_MAX_POINTS];
    size_t count;
    size_t index;
    float radius;
} BluewakePadRoute;

bool bluewake_pad_world_angle_steer(float world_angle,
                                    float camera_eye_x, float camera_eye_z,
                                    float camera_center_x,
                                    float camera_center_z, s8 magnitude,
                                    BluewakePadWaypointOutput* output);
bool bluewake_pad_waypoint_steer(float player_x, float player_z,
                                 float camera_eye_x, float camera_eye_z,
                                 float camera_center_x, float camera_center_z,
                                 float target_x, float target_z, s8 magnitude,
                                 BluewakePadWaypointOutput* output);
bool bluewake_pad_route_parse(const char* text, float radius,
                              BluewakePadRoute* route);
bool bluewake_pad_route_update(BluewakePadRoute* route, float player_x,
                               float player_z, float* target_x,
                               float* target_z);

#endif
