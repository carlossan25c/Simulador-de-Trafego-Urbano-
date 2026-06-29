#ifndef AMBULANCE_H
#define AMBULANCE_H

#include "types.h"
#include "constants.h"
#include "vehicle.h"
#include "traffic_light.h"

typedef struct {
    Vehicle      *vehicle;
    Map          *map;
    SimClock     *clock;
    TrafficLight *lights;
    int           n_lights;
} AmbulanceThreadArgs;

void ambulance_init(Vehicle *amb, int start_row, int start_col, int *route, int route_len);
void ambulance_request_priority(TrafficLight *tl, int dir, int tick, int amb_row, int amb_col);
void *ambulance_thread(void *arg);

#endif
