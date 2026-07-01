#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "constants.h"
#include "traffic_light.h"
#include "clock.h"
#include "vehicle.h"

void traffic_light_init(TrafficLight *tl, int id, int row, int col, int green_dur, int red_dur)
{
    tl->id                = id;
    tl->intersection_row  = row;
    tl->intersection_col  = col;
    tl->state_horizontal  = LIGHT_GREEN;
    tl->state_vertical    = LIGHT_RED;
    tl->green_duration    = green_dur;
    tl->red_duration      = red_dur;
    tl->elapsed_ticks     = 0;
    pthread_mutex_init(&tl->mutex, NULL);
    pthread_cond_init(&tl->cond, NULL);
}

void *traffic_light_thread(void *arg)
{
    TrafficLightThreadArgs *targs = (TrafficLightThreadArgs *)arg;
    TrafficLight *tl  = targs->light;
    SimClock     *clk = targs->clock;

    long last_tick   = clock_get_tick(clk);

    while (1) {
        clock_wait_tick(clk, last_tick);
        last_tick = clock_get_tick(clk);

        pthread_mutex_lock(&tl->mutex);
        tl->elapsed_ticks++;

        int current_duration;
        if (tl->state_horizontal == LIGHT_GREEN)
            current_duration = tl->green_duration;
        else
            current_duration = tl->red_duration;

        if (tl->elapsed_ticks >= current_duration) {
            tl->elapsed_ticks = 0;

            if (tl->state_horizontal == LIGHT_GREEN) {
                tl->state_horizontal = LIGHT_RED;
                tl->state_vertical   = LIGHT_GREEN;
            } else {
                tl->state_horizontal = LIGHT_GREEN;
                tl->state_vertical   = LIGHT_RED;
            }

            pthread_cond_broadcast(&tl->cond);
        }
        pthread_mutex_unlock(&tl->mutex);
    }
    return NULL;
}

void traffic_light_wait_green(TrafficLight *tl, int vehicle_dir)
{
    pthread_mutex_lock(&tl->mutex);
    if (vehicle_dir == DIR_EAST || vehicle_dir == DIR_WEST) {
        while (tl->state_horizontal == LIGHT_RED) {
            pthread_cond_wait(&tl->cond, &tl->mutex);
        }
    } else {
        while (tl->state_vertical == LIGHT_RED) {
            pthread_cond_wait(&tl->cond, &tl->mutex);
        }
    }
    pthread_mutex_unlock(&tl->mutex);
}

int traffic_light_is_green(TrafficLight *tl, int dir)
{
    int result;
    pthread_mutex_lock(&tl->mutex);
    if (dir == DIR_EAST || dir == DIR_WEST)
        result = (tl->state_horizontal == LIGHT_GREEN);
    else
        result = (tl->state_vertical == LIGHT_GREEN);
    pthread_mutex_unlock(&tl->mutex);
    return result;
}

void traffic_light_force_green(TrafficLight *tl, int dir)
{
    pthread_mutex_lock(&tl->mutex);
    if (dir == DIR_EAST || dir == DIR_WEST) {
        tl->state_horizontal = LIGHT_GREEN;
        tl->state_vertical   = LIGHT_RED;
    } else {
        tl->state_vertical   = LIGHT_GREEN;
        tl->state_horizontal = LIGHT_RED;
    }
    tl->elapsed_ticks = 0;
    pthread_cond_broadcast(&tl->cond);
    pthread_mutex_unlock(&tl->mutex);
}

void traffic_light_destroy(TrafficLight *tl)
{
    pthread_mutex_destroy(&tl->mutex);
    pthread_cond_destroy(&tl->cond);
}

int lights_init_all(TrafficLight lights[], int max_lights)
{
    static const int intersections[][2] = {
        { 5, 10}, { 5, 22}, { 5, 34}, { 5, 46},
        {11, 10}, {11, 22}, {11, 34}, {11, 46}
    };

    int n = sizeof(intersections) / sizeof(intersections[0]);
    if (n > max_lights)
        n = max_lights;

    for (int i = 0; i < n; i++) {
        traffic_light_init(&lights[i], i, intersections[i][0], intersections[i][1], DEFAULT_GREEN_TICKS, DEFAULT_RED_TICKS);
    }
    return n;
}
