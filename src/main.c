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

/* Rotas: pares {row, col} de cruzamentos a visitar em sequência circular */
static int route_amb[]  = { 5,10,  5,22,  5,34,  5,46,
                            11,46, 11,34, 11,22, 11,10 };

static int route_v1[]   = {  5,10,  11,10, 11,22,  5,22 };
static int route_v2[]   = {  5,22,   5,34, 11,34, 11,22 };
static int route_v3[]   = { 11,10,  11,22, 11,34, 11,46 };
static int route_v4[]   = {  5,46,  11,46, 11,34,  5,34 };
static int route_v5[]   = {  5,10,   5,22,  5,34,  5,46 };
static int route_v6[]   = { 11,22,   5,22,  5,10, 11,10 };
static int route_v7[]   = {  5,34,  11,34, 11,46,  5,46 };
static int route_v8[]   = { 11,46,  11,34, 11,22, 11,10 };
static int route_v9[]   = {  5,10,  11,10, 11,46,  5,46 };
static int route_v10[]  = {  5,22,  11,22, 11,10,  5,10 };
static int route_v11[]  = { 11,34,   5,34,  5,22, 11,22 };
static int route_v12[]  = {  5,46,   5,34,  5,22,  5,10 };
static int route_v13[]  = { 11,10,  11,22, 11,34, 11,46 };
static int route_v14[]  = {  5,34,   5,46, 11,46, 11,34 };