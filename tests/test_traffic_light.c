#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "types.h"
#include "constants.h"
#include "vehicle.h"
#include "traffic_light.h"
#include "ambulance.h"

#ifdef _WIN32
#include <windows.h>
#define msleep(ms) Sleep(ms)
#else
#include <unistd.h>
#define msleep(ms) usleep((ms) * 1000)
#endif

extern void  clock_init(SimClock *clk);
extern void *clock_thread(void *arg);
extern long  clock_get_tick(SimClock *clk);
extern void  clock_wait_tick(SimClock *clk, long last_tick);
extern void  clock_destroy(SimClock *clk);

extern void  map_init(Map *map);
extern void  map_destroy(Map *map);
extern void  cell_lock(Map *map, int row, int col);
extern void  cell_unlock(Map *map, int row, int col);
extern int   cell_is_free(Map *map, int row, int col);
extern void  cell_occupy(Map *map, int row, int col, int vehicle_id);
extern void  cell_release(Map *map, int row, int col);

static void test_1_lights_init_all(void)
{
    TrafficLight lights[NUM_INTERSECTIONS];
    int n = lights_init_all(lights, NUM_INTERSECTIONS);

    for (int i = 0; i < n; i++) {
        traffic_light_destroy(&lights[i]);
    }
}

typedef struct {
    TrafficLight *tl;
    int           vehicle_dir;
    int           vehicle_id;
} BlockedVehicleArgs;

static void *blocked_vehicle_fn(void *arg)
{
    BlockedVehicleArgs *bva = (BlockedVehicleArgs *)arg;
    traffic_light_wait_green(bva->tl, bva->vehicle_dir);
    return NULL;
}

static void test_2_force_green(void)
{
    TrafficLight tl;
    traffic_light_init(&tl, 0, 5, 10, 100, 100);
    pthread_mutex_lock(&tl.mutex);
    tl.state_horizontal = LIGHT_RED;
    tl.state_vertical   = LIGHT_GREEN;
    pthread_mutex_unlock(&tl.mutex);

    BlockedVehicleArgs bva = {
        .tl = &tl, .vehicle_dir = DIR_EAST, .vehicle_id = 99
    };

    pthread_t v_tid;
    pthread_create(&v_tid, NULL, blocked_vehicle_fn, &bva);

    msleep(100);
    traffic_light_force_green(&tl, DIR_EAST);

    pthread_join(v_tid, NULL);
    traffic_light_destroy(&tl);
}

static void test_3_semaforo_basico(void)
{
    static SimClock clk;
    clock_init(&clk);

    static TrafficLight tl;
    traffic_light_init(&tl, 0, 5, 10, 4, 4);

    pthread_t clk_tid;
    pthread_create(&clk_tid, NULL, clock_thread, &clk);
    pthread_detach(clk_tid);

    static TrafficLightThreadArgs tl_args;
    tl_args.light = &tl;
    tl_args.clock = &clk;
    pthread_t tl_tid;
    pthread_create(&tl_tid, NULL, traffic_light_thread, &tl_args);
    pthread_detach(tl_tid);

    msleep(100);
}

static void test_4_direcao_vertical(void)
{
    static SimClock clk;
    clock_init(&clk);

    static TrafficLight tl;
    traffic_light_init(&tl, 1, 5, 10, 3, 3);

    pthread_t clk_tid;
    pthread_create(&clk_tid, NULL, clock_thread, &clk);
    pthread_detach(clk_tid);

    static TrafficLightThreadArgs tl_args;
    tl_args.light = &tl;
    tl_args.clock = &clk;
    pthread_t tl_tid;
    pthread_create(&tl_tid, NULL, traffic_light_thread, &tl_args);
    pthread_detach(tl_tid);
}

static void test_5_ambulancia(void)
{
    static SimClock clk;
    clock_init(&clk);

    static Map map;
    map_init(&map);

    static TrafficLight tl;
    traffic_light_init(&tl, 0, 5, 10, 4, 4);
    pthread_mutex_lock(&tl.mutex);
    tl.state_horizontal = LIGHT_RED;
    tl.state_vertical   = LIGHT_GREEN;
    pthread_mutex_unlock(&tl.mutex);

    static Vehicle amb;
    ambulance_init(&amb, 5, 8, NULL, 0);
    amb.direction = DIR_EAST;

    cell_lock(&map, 5, 8);
    cell_occupy(&map, 5, 8, amb.id);
    cell_unlock(&map, 5, 8);

    pthread_t clk_tid;
    pthread_create(&clk_tid, NULL, clock_thread, &clk);
    pthread_detach(clk_tid);

    static TrafficLightThreadArgs tl_args;
    tl_args.light = &tl;
    tl_args.clock = &clk;
    pthread_t tl_tid;
    pthread_create(&tl_tid, NULL, traffic_light_thread, &tl_args);
    pthread_detach(tl_tid);

    static AmbulanceThreadArgs amb_args;
    amb_args.vehicle  = &amb;
    amb_args.map      = &map;
    amb_args.clock    = &clk;
    amb_args.lights   = &tl;
    amb_args.n_lights = 1;
    pthread_t amb_tid;
    pthread_create(&amb_tid, NULL, ambulance_thread, &amb_args);

    msleep(200);

    amb.active = 0;
    msleep(TICK_MS * 2);
    pthread_join(amb_tid, NULL);
}

int main(void)
{
    test_1_lights_init_all();
    test_2_force_green();
    test_3_semaforo_basico();
    test_4_direcao_vertical();
    test_5_ambulancia();

    printf("Todos os testes executados com sucesso!\n");
    exit(0);
    return 0;
}
