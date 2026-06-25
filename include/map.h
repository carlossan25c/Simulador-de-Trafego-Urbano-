#ifndef MAP_H
#define MAP_H

#include "types.h"

void map_init(Map *map);
void map_destroy(Map *map);

void cell_lock(Map *map, int row, int col);
void cell_unlock(Map *map, int row, int col);
int  cell_is_free(Map *map, int row, int col);
void cell_occupy(Map *map, int row, int col, int vehicle_id);
void cell_release(Map *map, int row, int col);

#endif
