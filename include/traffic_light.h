#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include "types.h"
#include "constants.h"

#define LIGHT_RED   0
#define LIGHT_GREEN 1

#ifndef NUM_INTERSECTIONS
#define NUM_INTERSECTIONS 8
#endif

#define DEFAULT_GREEN_TICKS  8
#define DEFAULT_RED_TICKS    8

#define AMBULANCE_PRIORITY_DISTANCE  2
/* ticks que o semáforo permanece bloqueado após force_green da ambulância */
#define AMBULANCE_PRIORITY_HOLD     6

typedef struct {
    TrafficLight *light;
    SimClock     *clock;
} TrafficLightThreadArgs;

void traffic_light_init(TrafficLight *tl, int id, int row, int col, int green_dur, int red_dur);
void *traffic_light_thread(void *arg);
void traffic_light_wait_green(TrafficLight *tl, int vehicle_dir);
int traffic_light_is_green(TrafficLight *tl, int dir);
void traffic_light_force_green(TrafficLight *tl, int dir);
void traffic_light_destroy(TrafficLight *tl);
int lights_init_all(TrafficLight lights[], int max_lights);

#endif
