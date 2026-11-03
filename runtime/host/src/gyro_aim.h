#ifndef BLUEWAKE_GYRO_AIM_H
#define BLUEWAKE_GYRO_AIM_H

// Gyro aiming (#188): a controller's gyroscope turns the aim in first person
// and when aiming an item, as the Wii U GamePad does in Wind Waker HD.
//
// SDL reports the gyro in radians a second around the controller's axes
// (SDL_sensor.h: +X to the right, +Y up, +Z toward the player, positive
// counterclockwise looking down the axis). So turning the controller left is
// +Y, and tilting its far end up is +X. The aim takes degrees, positive yaw
// turning right and positive pitch looking down (mouse_camera.c's aim_frame),
// so both come in negated: the view follows the controller one to one at
// speed 1.
static inline void bw_gyro_turn(double rate_x, double rate_y, double seconds, double speed, double* yaw,
                                double* pitch) {
    const double degrees = 57.29577951308232 * seconds * speed;
    *yaw -= rate_y * degrees;
    *pitch -= rate_x * degrees;
}

#endif
