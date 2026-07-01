#include <pthread.h>
#include "map.h"
#include "constants.h"

/* Faixas horizontais: eastbound_row e westbound_row (-1 = mão única) */
static const int H_EAST_ROWS[] = { H1_EAST, H2_EAST, H3_EAST };
static const int H_WEST_ROWS[] = { H1_WEST, H2_WEST, -1 };
#define N_H_STREETS 3

/* Faixas verticais: southbound_col e northbound_col */
static const int V_SOUTH_COLS[] = { V1_SOUTH, V2_SOUTH, V3_SOUTH, V4_SOUTH };
static const int V_NORTH_COLS[] = { V1_NORTH, V2_NORTH, V3_NORTH, V4_NORTH };
#define N_V_STREETS 4

void map_init(Map *map)
{
    map->rows = MAP_ROWS;
    map->cols = MAP_COLS;

    /* 1. Todas as células iniciam como parede */
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            Cell *cell    = &map->grid[r][c];
            cell->type       = CELL_WALL;
            cell->direction  = 0;
            cell->lane       = 0;
            cell->occupied   = 0;
            cell->vehicle_id = -1;
            pthread_mutex_init(&cell->mutex, NULL);
        }
    }

    /* 2. Faixas horizontais (cols 1..MAP_COLS-2 para deixar bordas como parede) */
    for (int i = 0; i < N_H_STREETS; i++) {
        int er = H_EAST_ROWS[i];
        int wr = H_WEST_ROWS[i];
        for (int c = 1; c < MAP_COLS - 1; c++) {
            map->grid[er][c].type      = CELL_ROAD;
            map->grid[er][c].direction = DIR_EAST;
        }
        if (wr >= 0) {
            for (int c = 1; c < MAP_COLS - 1; c++) {
                map->grid[wr][c].type      = CELL_ROAD;
                map->grid[wr][c].direction = DIR_WEST;
            }
        }
    }

    /* 3. Faixas verticais (rows 1..MAP_ROWS-2) */
    for (int j = 0; j < N_V_STREETS; j++) {
        int sc = V_SOUTH_COLS[j];
        int nc = V_NORTH_COLS[j];
        for (int r = 1; r < MAP_ROWS - 1; r++) {
            map->grid[r][sc].type      = CELL_ROAD;
            map->grid[r][sc].direction = DIR_SOUTH;
            map->grid[r][nc].type      = CELL_ROAD;
            map->grid[r][nc].direction = DIR_NORTH;
        }
    }

    /* 4. Cruzamentos: onde horizontal encontra vertical
     *    Mão dupla × mão dupla → bloco 2×2
     *    Mão única × mão dupla → bloco 1×2 */
    for (int i = 0; i < N_H_STREETS; i++) {
        for (int j = 0; j < N_V_STREETS; j++) {
            int er = H_EAST_ROWS[i];
            int wr = H_WEST_ROWS[i];
            int sc = V_SOUTH_COLS[j];
            int nc = V_NORTH_COLS[j];

            map->grid[er][sc].type = CELL_INTERSECTION;
            map->grid[er][nc].type = CELL_INTERSECTION;
            if (wr >= 0) {
                map->grid[wr][sc].type = CELL_INTERSECTION;
                map->grid[wr][nc].type = CELL_INTERSECTION;
            }
        }
    }
}

void map_destroy(Map *map)
{
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            pthread_mutex_destroy(&map->grid[r][c].mutex);
}

void cell_lock(Map *map, int row, int col)
{
    pthread_mutex_lock(&map->grid[row][col].mutex);
}

void cell_unlock(Map *map, int row, int col)
{
    pthread_mutex_unlock(&map->grid[row][col].mutex);
}

int cell_is_free(Map *map, int row, int col)
{
    return (map->grid[row][col].occupied == 0);
}

void cell_occupy(Map *map, int row, int col, int vehicle_id)
{
    map->grid[row][col].occupied   = 1;
    map->grid[row][col].vehicle_id = vehicle_id;
}

void cell_release(Map *map, int row, int col)
{
    map->grid[row][col].occupied   = 0;
    map->grid[row][col].vehicle_id = -1;
}
