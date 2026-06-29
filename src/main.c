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

static volatile sig_atomic_t g_running = 1;

static void handle_sigint(int sig)
{
    (void)sig;
    g_running = 0;

    /* Acorda threads que estão bloqueadas esperando o próximo tick */
    pthread_mutex_lock(&g_clock.mutex);
    pthread_cond_broadcast(&g_clock.cond);
    pthread_mutex_unlock(&g_clock.mutex);
}