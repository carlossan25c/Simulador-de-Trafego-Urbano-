#include <pthread.h>
#include "types.h"
#include "constants.h"

void map_init(Map *map);
void cell_lock(Map *map, int row, int col);
void cell_unlock(Map *map, int row, int col);
int  cell_is_free(Map *map, int row, int col);
void cell_occupy(Map *map, int row, int col, int vehicle_id);
void cell_release(Map *map, int row, int col);
void map_destroy(Map *map);

#define CELL_WALL         0
#define CELL_ROAD         1
#define CELL_INTERSECTION 2

void map_init(Map *map)
{
    map->rows = MAP_ROWS;
    map->cols = MAP_COLS;

    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            map->grid[r][c].type       = CELL_ROAD;
            map->grid[r][c].direction  = 0;
            map->grid[r][c].lane       = 0;
            map->grid[r][c].occupied   = 0;
            map->grid[r][c].vehicle_id = -1;
            pthread_mutex_init(&map->grid[r][c].mutex, NULL);
        }
    }
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

void map_destroy(Map *map)
{
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            pthread_mutex_destroy(&map->grid[r][c].mutex);
        }
    }
}
