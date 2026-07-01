#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
    pthread_mutex_lock(&g_clock.mutex);
    pthread_cond_broadcast(&g_clock.cond);
    pthread_mutex_unlock(&g_clock.mutex);
}

/* Rota fixa da ambulância (percorre o perímetro completo) */
static int route_amb[] = {
     5,23,  5,36,  5,49,
    13,49, 13,37, 13,24, 13,11,
     5,11
};

static TrafficLightThreadArgs g_light_args[NUM_INTERSECTIONS];

static void spawn_clock(void) {
    pthread_create(&g_clock_tid, NULL, clock_thread, &g_clock);
}

static void spawn_lights(void) {
    for (int i = 0; i < NUM_INTERSECTIONS; i++) {
        g_light_args[i].light = &g_lights[i];
        g_light_args[i].clock = &g_clock;
        pthread_create(&g_light_tids[i], NULL, traffic_light_thread, &g_light_args[i]);
    }
}

/*
 * Veículos espalhados por todas as faixas e com direções variadas.
 * Sem rotas fixas: cada um navega aleatoriamente a partir daqui.
 *
 * Faixas disponíveis:
 *   row  5 (H1_EAST, originalmente →)    row  6 (H1_WEST, originalmente ←)
 *   row 12 (H2_EAST, originalmente →)    row 13 (H2_WEST, originalmente ←)
 *   row 19 (H3_EAST, originalmente →)
 *   col 10 (V1_SOUTH ↓)  col 11 (V1_NORTH ↑)
 *   col 23 (V2_SOUTH ↓)  col 24 (V2_NORTH ↑)
 *   col 36 (V3_SOUTH ↓)  col 37 (V3_NORTH ↑)
 *   col 49 (V4_SOUTH ↓)  col 50 (V4_NORTH ↑)
 */
static const struct {
    int row, col, speed, direction;
} vehicle_cfg[NUM_VEHICLES] = {
    /* ID 0 — ambulância (rota fixa, tratada separadamente) */
    {  5,  2, SPEED_FAST,   DIR_EAST  },

    /* Faixas horizontais superiores — direções opostas */
    {  5, 14, SPEED_MEDIUM, DIR_EAST  },  /*  1: H1_EAST → */
    {  6, 30, SPEED_SLOW,   DIR_WEST  },  /*  2: H1_WEST ← */
    {  5, 42, SPEED_FAST,   DIR_EAST  },  /*  3: H1_EAST → */
    {  6,  8, SPEED_MEDIUM, DIR_WEST  },  /*  4: H1_WEST ← */

    /* Faixas horizontais inferiores — direções opostas */
    { 12, 18, SPEED_SLOW,   DIR_EAST  },  /*  5: H2_EAST → */
    { 13, 40, SPEED_FAST,   DIR_WEST  },  /*  6: H2_WEST ← */
    { 12, 38, SPEED_MEDIUM, DIR_EAST  },  /*  7: H2_EAST → (entre V3 e V4) */
    { 13, 15, SPEED_SLOW,   DIR_WEST  },  /*  8: H2_WEST ← */

    /* Faixa lateral (H3 — mão única para leste) */
    { 19, 28, SPEED_MEDIUM, DIR_EAST  },  /*  9: H3_EAST → */
    { 19,  4, SPEED_SLOW,   DIR_EAST  },  /* 10: H3_EAST → (começa na ponta oeste) */

    /* Faixas verticais — direções opostas */
    {  8, 10, SPEED_FAST,   DIR_SOUTH },  /* 11: V1_SOUTH ↓ */
    { 16, 11, SPEED_MEDIUM, DIR_NORTH },  /* 12: V1_NORTH ↑ */
    {  3, 23, SPEED_SLOW,   DIR_SOUTH },  /* 13: V2_SOUTH ↓ */
    { 17, 36, SPEED_FAST,   DIR_NORTH },  /* 14: V3_NORTH ↑ */
};

static VehicleThreadArgs   g_vehicle_args[NUM_VEHICLES];
static AmbulanceThreadArgs g_amb_args;

static void spawn_vehicles(void)
{
    /* Ambulância — ID 0 com rota fixa */
    ambulance_init(&g_vehicles[0],
                   vehicle_cfg[0].row, vehicle_cfg[0].col,
                   route_amb, 16);
    cell_occupy(&g_map, vehicle_cfg[0].row, vehicle_cfg[0].col, 0);

    g_amb_args.vehicle  = &g_vehicles[0];
    g_amb_args.map      = &g_map;
    g_amb_args.clock    = &g_clock;
    g_amb_args.lights   = g_lights;
    g_amb_args.n_lights = NUM_INTERSECTIONS;
    pthread_create(&g_vehicle_tids[0], NULL, ambulance_thread, &g_amb_args);

    /* Veículos comuns — navegação aleatória */
    for (int i = 1; i < NUM_VEHICLES; i++) {
        vehicle_init(&g_vehicles[i], i,
                     vehicle_cfg[i].row, vehicle_cfg[i].col,
                     vehicle_cfg[i].speed,
                     vehicle_cfg[i].direction);
        cell_occupy(&g_map, vehicle_cfg[i].row, vehicle_cfg[i].col, i);

        g_vehicle_args[i].vehicle  = &g_vehicles[i];
        g_vehicle_args[i].map      = &g_map;
        g_vehicle_args[i].clock    = &g_clock;
        g_vehicle_args[i].lights   = g_lights;
        g_vehicle_args[i].n_lights = NUM_INTERSECTIONS;
        pthread_create(&g_vehicle_tids[i], NULL, vehicle_thread, &g_vehicle_args[i]);
    }
}

int main(void)
{
    srand((unsigned int)time(NULL));

    render_init();
    map_init(&g_map);
    clock_init(&g_clock);
    lights_init_all(g_lights, NUM_INTERSECTIONS);

    signal(SIGINT, handle_sigint);

    spawn_clock();
    spawn_lights();
    spawn_vehicles();

    long last_tick = clock_get_tick(&g_clock);
    while (g_running) {
        clock_wait_tick(&g_clock, last_tick);
        if (!g_running) break;
        last_tick = clock_get_tick(&g_clock);
        render_frame(&g_map, g_vehicles, NUM_VEHICLES,
                     g_lights, NUM_INTERSECTIONS, last_tick);
    }

    for (int i = 0; i < NUM_VEHICLES; i++)
        g_vehicles[i].active = 0;

    pthread_cancel(g_clock_tid);
    pthread_join(g_clock_tid, NULL);

    for (int i = 0; i < NUM_INTERSECTIONS; i++) {
        pthread_cancel(g_light_tids[i]);
        pthread_join(g_light_tids[i], NULL);
    }

    for (int i = 0; i < NUM_VEHICLES; i++)
        pthread_join(g_vehicle_tids[i], NULL);

    for (int i = 0; i < NUM_INTERSECTIONS; i++)
        traffic_light_destroy(&g_lights[i]);
    clock_destroy(&g_clock);
    map_destroy(&g_map);
    render_destroy();

    return 0;
}
