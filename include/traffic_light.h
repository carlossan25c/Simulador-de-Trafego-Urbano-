#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include "types.h"

void  traffic_light_init(TrafficLight *tl, int id, int row, int col,
                         int green_dur, int red_dur);
void  traffic_light_destroy(TrafficLight *tl);
void *traffic_light_thread(void *arg);

void  traffic_light_wait_green(TrafficLight *tl, int direction);
int   traffic_light_is_green(TrafficLight *tl, int direction);
void  traffic_light_force_green(TrafficLight *tl, int direction);

#endif
