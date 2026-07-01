#include <stdlib.h>
#include "types.h"
#include "constants.h"
#include "vehicle.h"

void vehicle_init(Vehicle *v, int id, int start_row, int start_col, int speed, int *route, int route_len)
{
    v->id            = id;
    v->row           = start_row;
    v->col           = start_col;
    v->direction     = DIR_EAST;
    v->speed         = speed;
    v->ticks_to_move = speed;
    v->active        = 1;
    v->route         = route;
    v->route_len     = route_len;
    v->route_idx     = 0;
}

int vehicle_is_ambulance(Vehicle *v)
{
    return (v->id == AMBULANCE_ID);
}

int get_next_cell(Vehicle *v, int *next_row, int *next_col)
{
    *next_row = v->row;
    *next_col = v->col;

    switch (v->direction) {
        case DIR_NORTH: (*next_row)--; break;
        case DIR_SOUTH: (*next_row)++; break;
        case DIR_EAST:  (*next_col)++; break;
        case DIR_WEST:  (*next_col)--; break;
        default: return -1;
    }

    if (*next_row < 0 || *next_row >= MAP_ROWS ||
        *next_col < 0 || *next_col >= MAP_COLS)
        return -1;

    return 0;
}
