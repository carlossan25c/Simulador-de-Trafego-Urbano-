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

/*
 * Todos os loops circulam no mesmo sentido horário:
 *   row5 → leste | col → sul | row11 → oeste | col → norte
 * Isso elimina tráfego oposto nas vias e previne deadlock de ocupação.
 *
 * route_A: loop pequeno esquerdo   (5,10)→(5,22)→(11,22)→(11,10)
 * route_B: loop médio              (5,22)→(5,46)→(11,46)→(11,22)
 * route_C: loop grande             (5,10)→(5,46)→(11,46)→(11,10)
 * route_D: loop pequeno direito    (5,34)→(5,46)→(11,46)→(11,34)
 */
static int route_amb[] = {  5,10,  5,22,  5,34,  5,46,
                            11,46, 11,34, 11,22, 11,10 };

static int route_A[]   = {  5,10,  5,22, 11,22, 11,10 };
static int route_B[]   = {  5,22,  5,46, 11,46, 11,22 };
static int route_C[]   = {  5,10,  5,46, 11,46, 11,10 };
static int route_D[]   = {  5,34,  5,46, 11,46, 11,34 };

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

/*
 * start_idx: índice inicial em route[] (múltiplo de 2).
 * Veículos são colocados em pontos distintos do loop para distribuição imediata.
 * row5 sempre leste, row11 sempre oeste — sem tráfego frontal.
 */
static const struct {
    int  row, col, speed;
    int *route;
    int  route_len;
    int  start_idx;
} vehicle_cfg[NUM_VEHICLES] = {
    /* ID 0 — ambulância: perimetro completo */
    {  5,  1, SPEED_FAST,   route_amb, 16, 0 },
    /* Loop A (pequeno esq): 3 veículos espalhados pelo circuito */
    {  5,  4, SPEED_MEDIUM, route_A, 8, 0 },  /*  1: row5, → (5,10)  */
    { 11, 17, SPEED_SLOW,   route_A, 8, 6 },  /*  2: row11, → (11,10) */
    {  7, 22, SPEED_FAST,   route_A, 8, 4 },  /*  3: col22, ↓ (11,22) */
    /* Loop B (médio): 3 veículos */
    {  5, 27, SPEED_SLOW,   route_B, 8, 2 },  /*  4: row5, → (5,46)  */
    {  7, 46, SPEED_FAST,   route_B, 8, 4 },  /*  5: col46, ↓ (11,46) */
    { 11, 33, SPEED_MEDIUM, route_B, 8, 6 },  /*  6: row11, → (11,22) */
    /* Loop C (grande): 4 veículos */
    {  5,  8, SPEED_FAST,   route_C, 8, 0 },  /*  7: row5, → (5,10)  */
    {  5, 34, SPEED_SLOW,   route_C, 8, 2 },  /*  8: row5, → (5,46)  */
    { 11, 50, SPEED_MEDIUM, route_C, 8, 6 },  /*  9: row11, → (11,10) */
    { 11, 27, SPEED_FAST,   route_C, 8, 6 },  /* 10: row11, → (11,10) */
    /* Loop D (pequeno dir): 4 veículos */
    {  5, 29, SPEED_MEDIUM, route_D, 8, 0 },  /* 11: row5, → (5,34)  */
    {  5, 41, SPEED_SLOW,   route_D, 8, 2 },  /* 12: row5, → (5,46)  */
    { 10, 46, SPEED_FAST,   route_D, 8, 4 },  /* 13: col46, ↓ (11,46) */
    { 11, 40, SPEED_MEDIUM, route_D, 8, 6 },  /* 14: row11, → (11,34) */
};

static void spawn_vehicles(void)
{
    /* Ambulância — ID 0 */
    ambulance_init(&g_vehicles[0],
                   vehicle_cfg[0].row, vehicle_cfg[0].col,
                   vehicle_cfg[0].route, vehicle_cfg[0].route_len);
    g_vehicles[0].route_idx = vehicle_cfg[0].start_idx;
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
        g_vehicles[i].route_idx = vehicle_cfg[i].start_idx;
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