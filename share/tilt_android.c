/*
 * Copyright (C) 2003 Robert Kooima
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include <math.h>
#include <android/sensor.h>

static ASensorManager *mgr = NULL;
static const ASensor *sensor = NULL;
static ALooper *looper = NULL;
static ASensorEventQueue *queue = NULL;

static float tilt_x = 0.0f;
static float tilt_z = 0.0f;

void tilt_init(void)
{
    const int refresh_hz = 100;

    mgr = ASensorManager_getInstance();
    sensor = ASensorManager_getDefaultSensor(mgr, ASENSOR_TYPE_ACCELEROMETER);
    if (sensor != NULL)
    {
        looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        queue = ASensorManager_createEventQueue(mgr, looper, 3, NULL, NULL);
        ASensorEventQueue_enableSensor(queue, sensor);
        ASensorEventQueue_setEventRate(queue, sensor, 1000000 / refresh_hz);
    }
}

void tilt_free(void)
{
}

int tilt_stat(void)
{
    /* This seems like a good place to poll the accelerometer. */
    if (sensor != NULL)
    {
        const float ALPHA = 0.5f;
        ASensorEvent event;
        float x, y, z;

        ALooper_pollAll(0, NULL, NULL, NULL);
        while (ASensorEventQueue_getEvents(queue, &event, 1) > 0)
        {
            /* Use a low pass filter to smooth out the jitter. */
            x = x + ALPHA * (event.acceleration.x - x);
            y = y + ALPHA * (event.acceleration.y - y);
            z = z + ALPHA * (event.acceleration.z - z);
        }

        /* Assumes upright landscape mode with bottom of screen pointing right */
        
        /* angle between gravity vector and x axis on xz plane */
        tilt_x = -atanf(-z / x);
        /* angle between gravity vector and y axis on xy plane */
        tilt_z = atanf(y / x);
        
        /* Convert to degrees */
        tilt_x = tilt_x * 180.0f / M_PI;
        tilt_z = tilt_z * 180.0f / M_PI;
        
        /* Make it so that the neutral position is with the screen tilted forward 45 degrees. */
        tilt_x -= 45.0f;

        /* Default was too sensitive */
        tilt_x /= 2.0f;
        tilt_z /= 2.0f;
        
        return 1;
    }

    return 0;
}

int  tilt_get_button(int *b, int *s)
{
    return 0;
}

float tilt_get_x(void)
{
    return tilt_x;
}

float tilt_get_z(void)
{
    return tilt_z;
}
