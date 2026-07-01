#include <pthread.h>
#include "map.h"
#include "constants.h"

/* Layout do mapa — linhas e colunas das vias */
static const int H_ROWS[]  = {5, 11, 17};   /* ruas horizontais */
static const int V_COLS[]  = {10, 22, 34, 46}; /* ruas verticais */
#define N_H_ROWS 3
#define N_V_COLS 4
#define H_ONE_WAY_ROW 17  /* linha 17: mão única → leste */

void map_init(Map *map)
{
    map->rows = MAP_ROWS;
    map->cols = MAP_COLS;

    /* 1. Inicializa todas as células como parede */
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

    /* 2. Ruas horizontais */
    for (int i = 0; i < N_H_ROWS; i++) {
        int r  = H_ROWS[i];
        int dir = (r == H_ONE_WAY_ROW) ? DIR_EAST : DIR_HORIZONTAL;
        for (int c = 1; c < MAP_COLS - 1; c++) {
            map->grid[r][c].type      = CELL_ROAD;
            map->grid[r][c].direction = dir;
        }
    }

    /* 3. Ruas verticais */
    for (int j = 0; j < N_V_COLS; j++) {
        int col = V_COLS[j];
        for (int r = 1; r < MAP_ROWS - 1; r++) {
            map->grid[r][col].type      = CELL_ROAD;
            map->grid[r][col].direction = DIR_VERTICAL;
        }
    }

    /* 4. Cruzamentos — onde horizontal encontra vertical */
    for (int i = 0; i < N_H_ROWS; i++) {
        for (int j = 0; j < N_V_COLS; j++) {
            map->grid[H_ROWS[i]][V_COLS[j]].type = CELL_INTERSECTION;
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
