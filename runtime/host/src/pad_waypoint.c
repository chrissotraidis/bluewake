#include "pad_waypoint.h"

#include <math.h>
#include <stdlib.h>

bool bluewake_pad_world_angle_steer(float world_angle,
                                    float camera_eye_x, float camera_eye_z,
                                    float camera_center_x,
                                    float camera_center_z, s8 magnitude,
                                    BluewakePadWaypointOutput* output) {
    if (output == NULL || magnitude <= 0)
        return false;
    const float camera_dx = camera_center_x - camera_eye_x;
    const float camera_dz = camera_center_z - camera_eye_z;
    if (hypotf(camera_dx, camera_dz) < 0.001f)
        return false;

    const float camera_angle = atan2f(camera_dx, camera_dz);
    const float raw_angle = world_angle - camera_angle - (float)M_PI;
    output->stick_x = (s8)lroundf((float)magnitude * sinf(raw_angle));
    output->stick_y = (s8)lroundf(-(float)magnitude * cosf(raw_angle));
    output->distance = 0.0f;
    return true;
}

bool bluewake_pad_waypoint_steer(float player_x, float player_z,
                                 float camera_eye_x, float camera_eye_z,
                                 float camera_center_x, float camera_center_z,
                                 float target_x, float target_z, s8 magnitude,
                                 BluewakePadWaypointOutput* output) {
    if (output == NULL || magnitude <= 0)
        return false;

    const float target_dx = target_x - player_x;
    const float target_dz = target_z - player_z;
    const float distance = hypotf(target_dx, target_dz);
    if (distance < 0.001f)
        return false;

    const float world_angle = atan2f(target_dx, target_dz);
    if (!bluewake_pad_world_angle_steer(
            world_angle, camera_eye_x, camera_eye_z, camera_center_x,
            camera_center_z, magnitude, output))
        return false;
    output->distance = distance;
    return true;
}

bool bluewake_pad_route_parse(const char* text, float radius,
                              BluewakePadRoute* route) {
    if (text == NULL || text[0] == '\0' || route == NULL ||
        !isfinite(radius) || radius <= 0.0f)
        return false;

    BluewakePadRoute parsed = {.radius = radius};
    const char* cursor = text;
    while (*cursor != '\0') {
        if (parsed.count >= BLUEWAKE_PAD_ROUTE_MAX_POINTS)
            return false;

        char* end = NULL;
        const float x = strtof(cursor, &end);
        if (end == cursor || !isfinite(x) || *end != ',')
            return false;
        cursor = end + 1;

        const float z = strtof(cursor, &end);
        if (end == cursor || !isfinite(z) || (*end != ';' && *end != '\0'))
            return false;
        parsed.points[parsed.count++] = (BluewakePadRoutePoint){x, z};
        if (*end == '\0')
            break;
        cursor = end + 1;
        if (*cursor == '\0')
            return false;
    }

    *route = parsed;
    return true;
}

bool bluewake_pad_route_update(BluewakePadRoute* route, float player_x,
                               float player_z, float* target_x,
                               float* target_z) {
    if (route == NULL || target_x == NULL || target_z == NULL ||
        route->count == 0u || route->index > route->count ||
        !isfinite(player_x) || !isfinite(player_z))
        return false;

    while (route->index < route->count) {
        const BluewakePadRoutePoint* point = &route->points[route->index];
        if (hypotf(point->x - player_x, point->z - player_z) > route->radius) {
            *target_x = point->x;
            *target_z = point->z;
            return true;
        }
        route->index++;
    }
    return false;
}
