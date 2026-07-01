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

static TrafficLightThreadArgs g_light_args[NUM_INTERSECTIONS];

static void spawn_clock(void)
{
    pthread_create(&g_clock_tid, NULL, clock_thread, &g_clock);
}

static void spawn_lights(void)
{
    for (int i = 0; i < NUM_INTERSECTIONS; i++) {
        g_light_args[i].light = &g_lights[i];
        g_light_args[i].clock = &g_clock;
        pthread_create(&g_light_tids[i], NULL, traffic_light_thread, &g_light_args[i]);
    }
}

static VehicleThreadArgs   g_vehicle_args[NUM_VEHICLES];
static AmbulanceThreadArgs g_amb_args;

/* Tabela de configuração: { start_row, start_col, speed, route*, route_len } */
static const struct {
    int  row, col, speed;
    int *route;
    int  route_len;
} vehicle_cfg[NUM_VEHICLES] = {
    /* ID 0 — ambulância (posição e rota próprias) */
    {  5,  1, SPEED_FAST,   route_amb, 16 },
    /* ID 1–14 — veículos comuns */
    {  5,  2, SPEED_MEDIUM, route_v1,   8 },
    {  5,  3, SPEED_SLOW,   route_v2,   8 },
    { 11,  2, SPEED_FAST,   route_v3,   8 },
    {  5, 58, SPEED_MEDIUM, route_v4,   8 },
    {  5,  4, SPEED_SLOW,   route_v5,   8 },
    { 11,  3, SPEED_FAST,   route_v6,   8 },
    {  5, 57, SPEED_MEDIUM, route_v7,   8 },
    { 11, 58, SPEED_SLOW,   route_v8,   8 },
    {  5,  5, SPEED_FAST,   route_v9,   8 },
    {  5, 23, SPEED_MEDIUM, route_v10,  8 },
    { 11, 35, SPEED_SLOW,   route_v11,  8 },
    {  5, 47, SPEED_FAST,   route_v12,  8 },
    { 11,  4, SPEED_MEDIUM, route_v13,  8 },
    {  5, 35, SPEED_SLOW,   route_v14,  8 },
};

static void spawn_vehicles(void)
{
    /* Ambulância — ID 0 */
    ambulance_init(&g_vehicles[0],
                   vehicle_cfg[0].row, vehicle_cfg[0].col,
                   vehicle_cfg[0].route, vehicle_cfg[0].route_len);
    cell_occupy(&g_map, vehicle_cfg[0].row, vehicle_cfg[0].col, 0);

    g_amb_args.vehicle  = &g_vehicles[0];
    g_amb_args.map      = &g_map;
    g_amb_args.clock    = &g_clock;
    g_amb_args.lights   = g_lights;
    g_amb_args.n_lights = NUM_INTERSECTIONS;
    pthread_create(&g_vehicle_tids[0], NULL, ambulance_thread, &g_amb_args);

    /* Veículos comuns — IDs 1–14 */
    for (int i = 1; i < NUM_VEHICLES; i++) {
        vehicle_init(&g_vehicles[i], i,
                     vehicle_cfg[i].row, vehicle_cfg[i].col,
                     vehicle_cfg[i].speed,
                     vehicle_cfg[i].route, vehicle_cfg[i].route_len);
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
    /* 1. Inicializa subsistemas */
    render_init();
    map_init(&g_map);
    clock_init(&g_clock);
    lights_init_all(g_lights, NUM_INTERSECTIONS);

    /* 2. Registra handler de SIGINT */
    signal(SIGINT, handle_sigint);

    /* 3. Sobe todas as threads */
    spawn_clock();
    spawn_lights();
    spawn_vehicles();

    /* 4. Loop de render — roda na main até Ctrl+C */
    long last_tick = clock_get_tick(&g_clock);
    while (g_running) {
        clock_wait_tick(&g_clock, last_tick);
        if (!g_running) break;
        last_tick = clock_get_tick(&g_clock);
        render_frame(&g_map, g_vehicles, NUM_VEHICLES, g_lights, NUM_INTERSECTIONS, last_tick);
    }

    /* 5. Shutdown: sinaliza veículos e aguarda todas as threads */
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

    /* 6. Libera recursos */
    for (int i = 0; i < NUM_INTERSECTIONS; i++)
        traffic_light_destroy(&g_lights[i]);
    clock_destroy(&g_clock);
    map_destroy(&g_map);
    render_destroy();

    return 0;
}