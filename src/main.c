#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include "constants.h"
#include "types.h"
#include "map.h"
#include "clock.h"
#include "vehicle.h"
#include "traffic_light.h"
#include "ambulance.h"
#include "render.h"

static Map          g_map;
static SimClock     g_clock;
static Vehicle      g_vehicles[NUM_VEHICLES];
static TrafficLight g_lights[NUM_INTERSECTIONS];

static pthread_t g_clock_tid;
static pthread_t g_light_tids[NUM_INTERSECTIONS];
static pthread_t g_vehicle_tids[NUM_VEHICLES];