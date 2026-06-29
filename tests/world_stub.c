/*
 * tests/stubs/world_stub.c
 *
 * Popula um Map, array de Vehicle e array de TrafficLight com valores
 * fixos e fictícios — sem threads, sem sincronização real.
 * Permite testar render_frame 100% isolado, sem depender de nenhum
 * outro módulo do projeto.
 *
 * Compilar:
 *   gcc -Wall -Wextra -pthread \
 *       world_stub.c ../../src/render.c \
 *       -I../../include \
 *       -o test_render
 *
 * Executar:
 *   ./test_render
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "../include/types.h"
#include "../include/constants.h"
#include "../include/render.h"

#define CELL_WALL         0
#define CELL_ROAD_H       1
#define CELL_ROAD_V       2
#define CELL_INTERSECTION 3
#define CELL_ONE_WAY_S    5
#define CELL_ONE_WAY_E    6

#define STUB_N_LIGHTS  4

static void init_stub_map(Map *map) {
    map->rows = MAP_ROWS;
    map->cols = MAP_COLS;

    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            map->grid[r][c].type       = CELL_WALL;
            map->grid[r][c].direction  = 0;
            map->grid[r][c].lane       = 0;
            map->grid[r][c].occupied   = 0;
            map->grid[r][c].vehicle_id = -1;
            pthread_mutex_init(&map->grid[r][c].mutex, NULL);
        }
    }

    int h_rows[] = {4, 10, 16};
    for (int ri = 0; ri < 3; ri++) {
        int r = h_rows[ri];
        for (int c = 0; c < MAP_COLS; c++)
            map->grid[r][c].type = CELL_ROAD_H;
    }

    int v_cols[] = {10, 25, 40, 55};
    for (int ci = 0; ci < 4; ci++) {
        int col = v_cols[ci];
        for (int r = 0; r < MAP_ROWS; r++)
            map->grid[r][col].type = CELL_ROAD_V;
    }

    for (int ri = 0; ri < 3; ri++)
        for (int ci = 0; ci < 4; ci++)
            map->grid[h_rows[ri]][v_cols[ci]].type = CELL_INTERSECTION;

    for (int r = 0; r < 4; r++)
        map->grid[r][55].type = CELL_ONE_WAY_S;

    for (int c = 56; c < MAP_COLS; c++)
        map->grid[4][c].type = CELL_ONE_WAY_E;
}

static void init_stub_vehicles(Vehicle *vehicles, int n) {
    vehicles[0].id           = 0;
    vehicles[0].row          = 4;
    vehicles[0].col          = 12;
    vehicles[0].direction    = 1;
    vehicles[0].speed        = SPEED_FAST;
    vehicles[0].ticks_to_move = 1;
    vehicles[0].active       = 1;
    vehicles[0].route        = NULL;
    vehicles[0].route_len    = 0;
    vehicles[0].route_idx    = 0;

    int start_cols[] = {3, 15, 20, 30, 35, 45, 50};
    for (int i = 1; i < n && i <= 7; i++) {
        vehicles[i].id           = i;
        vehicles[i].row          = 10;
        vehicles[i].col          = start_cols[i - 1];
        vehicles[i].direction    = 1;
        vehicles[i].speed        = (i % 2 == 0) ? SPEED_MEDIUM : SPEED_SLOW;
        vehicles[i].ticks_to_move = 2;
        vehicles[i].active       = 1;
        vehicles[i].route        = NULL;
        vehicles[i].route_len    = 0;
        vehicles[i].route_idx    = 0;
    }

    for (int i = 8; i < n; i++) {
        vehicles[i].id           = i;
        vehicles[i].row          = 2 + (i - 8) * 2;
        vehicles[i].col          = 25;
        vehicles[i].direction    = 2;
        vehicles[i].speed        = SPEED_FAST;
        vehicles[i].ticks_to_move = 1;
        vehicles[i].active       = (i < 12) ? 1 : 0;
        vehicles[i].route        = NULL;
        vehicles[i].route_len    = 0;
        vehicles[i].route_idx    = 0;
    }
}

static void init_stub_lights(TrafficLight *lights) {
    int rows[] = {4,  4, 10, 16};
    int cols[] = {10, 25, 40, 55};

    for (int i = 0; i < STUB_N_LIGHTS; i++) {
        lights[i].id               = i;
        lights[i].intersection_row = rows[i];
        lights[i].intersection_col = cols[i];
        lights[i].state_horizontal = (i % 2 == 0) ? 1 : 0;
        lights[i].state_vertical   = (i % 2 == 0) ? 0 : 1;
        lights[i].green_duration   = 5;
        lights[i].red_duration     = 5;
        pthread_mutex_init(&lights[i].mutex, NULL);
        pthread_cond_init(&lights[i].cond, NULL);
    }
}

static void stub_tick_vehicles(Vehicle *vehicles, int n, Map *map) {
    for (int i = 0; i < n; i++) {
        if (!vehicles[i].active) continue;
        if (vehicles[i].direction == 1) {
            int nc = vehicles[i].col + 1;
            if (nc >= MAP_COLS) nc = 1;
            vehicles[i].col = nc;
        } else if (vehicles[i].direction == 2) {
            int nr = vehicles[i].row + 1;
            if (nr >= MAP_ROWS) nr = 0;
            vehicles[i].row = nr;
        }
        (void)map;
    }
}

static void stub_tick_lights(TrafficLight *lights, int n, long tick) {
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(&lights[i].mutex);
        int fase = (int)(tick / 5) % 2;
        lights[i].state_horizontal = (i % 2 == 0) ? fase : (1 - fase);
        lights[i].state_vertical   = 1 - lights[i].state_horizontal;
        pthread_mutex_unlock(&lights[i].mutex);
    }
}

int main(void) {
    Map          map;
    Vehicle      vehicles[NUM_VEHICLES];
    TrafficLight lights[STUB_N_LIGHTS];

    init_stub_map(&map);
    init_stub_vehicles(vehicles, NUM_VEHICLES);
    init_stub_lights(lights);
    render_init();

    render_log("[TICK  1] Ambulância solicitou prioridade no cruzamento (4,25)");
    render_log("[TICK  3] Veículo 3 aguardando sinal vermelho em (10,40)");

    for (long tick = 1; tick <= 30; tick++) {
        stub_tick_vehicles(vehicles, NUM_VEHICLES, &map);
        stub_tick_lights(lights, STUB_N_LIGHTS, tick);

        if (tick % 8 == 0) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "[TICK %2ld] Sinal alterado no cruzamento (10,40)", tick);
            render_log(msg);
        }

        render_frame(&map, vehicles, NUM_VEHICLES,
                     lights, STUB_N_LIGHTS, tick);

        usleep(TICK_MS * 1000);
    }

    render_log("[FIM] Simulação encerrada após 30 ticks.");
    render_frame(&map, vehicles, NUM_VEHICLES, lights, STUB_N_LIGHTS, 30);

    render_destroy();

    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            pthread_mutex_destroy(&map.grid[r][c].mutex);

    for (int i = 0; i < STUB_N_LIGHTS; i++) {
        pthread_mutex_destroy(&lights[i].mutex);
        pthread_cond_destroy(&lights[i].cond);
    }

    return 0;
}