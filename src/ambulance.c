#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "constants.h"
#include "vehicle.h"
#include "traffic_light.h"
#include "ambulance.h"
#include "map.h"
#include "clock.h"

static void ambulance_update_direction_to_target(Vehicle *v)
{
    if (v->route == NULL || v->route_len < 2)
        return;

    int target_row = v->route[v->route_idx];
    int target_col = v->route[v->route_idx + 1];

    if (target_row < v->row) {
        v->direction = DIR_NORTH;
    } else if (target_row > v->row) {
        v->direction = DIR_SOUTH;
    } else if (target_col > v->col) {
        v->direction = DIR_EAST;
    } else if (target_col < v->col) {
        v->direction = DIR_WEST;
    }
}

void ambulance_init(Vehicle *amb, int start_row, int start_col, int *route, int route_len)
{
    amb->id            = AMBULANCE_ID;
    amb->row           = start_row;
    amb->col           = start_col;
    amb->direction     = DIR_EAST;
    amb->speed         = SPEED_FAST;
    amb->ticks_to_move = SPEED_FAST;
    amb->active        = 1;
    amb->route         = route;
    amb->route_len     = route_len;
    amb->route_idx     = 0;
    amb->stuck_ticks   = 0;
}

void ambulance_request_priority(TrafficLight *tl, int dir, int tick, int amb_row, int amb_col)
{
    traffic_light_force_green(tl, dir);
    printf("[TICK %d] AMBULANCIA ID=%d solicitou prioridade no cruzamento (%d,%d) - forcou verde para %s\n",
           tick, AMBULANCE_ID, tl->intersection_row, tl->intersection_col,
           (dir == DIR_EAST || dir == DIR_WEST) ? "HORIZONTAL" : "VERTICAL");
    (void)amb_row;
    (void)amb_col;
}

static TrafficLight *find_light_at(TrafficLight *lights, int n_lights, int row, int col)
{
    for (int i = 0; i < n_lights; i++) {
        int lr = lights[i].intersection_row;
        int lc = lights[i].intersection_col;
        /* Cruzamentos são blocos 2×2: rows lr,lr+1 e cols lc,lc+1 */
        if ((row == lr || row == lr + 1) && (col == lc || col == lc + 1))
            return &lights[i];
    }
    return NULL;
}

static TrafficLight *find_light_ahead(TrafficLight *lights, int n_lights, int row, int col, int dir, int max_dist)
{
    int dr = 0, dc = 0;
    switch (dir) {
        case DIR_NORTH: dr = -1; break;
        case DIR_SOUTH: dr =  1; break;
        case DIR_EAST:  dc =  1; break;
        case DIR_WEST:  dc = -1; break;
    }

    for (int d = 1; d <= max_dist; d++) {
        int r = row + dr * d;
        int c = col + dc * d;
        if (r < 0 || r >= MAP_ROWS || c < 0 || c >= MAP_COLS)
            break;

        TrafficLight *tl = find_light_at(lights, n_lights, r, c);
        if (tl != NULL)
            return tl;
    }
    return NULL;
}

void *ambulance_thread(void *arg)
{
    AmbulanceThreadArgs *args = (AmbulanceThreadArgs *)arg;
    Vehicle      *v      = args->vehicle;
    Map          *map    = args->map;
    SimClock     *clk    = args->clock;
    TrafficLight *lights = args->lights;
    int           n_lights = args->n_lights;

    long last_tick = clock_get_tick(clk);

    if (v->route != NULL && v->route_len >= 2) {
        if (v->row == v->route[v->route_idx] &&
            v->col == v->route[v->route_idx + 1]) {
            v->route_idx = (v->route_idx + 2) % v->route_len;
        }
        ambulance_update_direction_to_target(v);
    }

    while (v->active) {
        clock_wait_tick(clk, last_tick);
        last_tick = clock_get_tick(clk);

        v->ticks_to_move--;
        if (v->ticks_to_move > 0)
            continue;
        v->ticks_to_move = v->speed;

        ambulance_update_direction_to_target(v);
        int nr, nc;
        if (get_next_cell(v, &nr, &nc) != 0) {
            continue;
        }

        /*
         * Força verde apenas se a próxima célula está livre.
         * Se há um veículo na interseção esperando a direção perpendicular,
         * forçar verde na direção da ambulância reseta elapsed_ticks e impede
         * esse veículo de receber o verde de que precisa (livelock).
         */
        cell_lock(map, nr, nc);
        int next_free = cell_is_free(map, nr, nc);
        cell_unlock(map, nr, nc);

        if (next_free) {
            TrafficLight *tl_ahead = find_light_ahead(lights, n_lights, v->row, v->col, v->direction, AMBULANCE_PRIORITY_DISTANCE);
            if (tl_ahead != NULL) {
                ambulance_request_priority(tl_ahead, v->direction, (int)last_tick, v->row, v->col);
            }
        }

        TrafficLight *tl = find_light_at(lights, n_lights, nr, nc);
        if (tl != NULL) {
            traffic_light_wait_green(tl, v->direction);
        }

        int cur_idx  = v->row * MAP_COLS + v->col;
        int next_idx = nr     * MAP_COLS + nc;

        if (cur_idx < next_idx) {
            cell_lock(map, v->row, v->col);
            cell_lock(map, nr, nc);
        } else {
            cell_lock(map, nr, nc);
            cell_lock(map, v->row, v->col);
        }

        if (!cell_is_free(map, nr, nc)) {
            cell_unlock(map, v->row, v->col);
            cell_unlock(map, nr, nc);
            continue;
        }

        cell_occupy(map, nr, nc, v->id);
        cell_release(map, v->row, v->col);

        cell_unlock(map, v->row, v->col);
        cell_unlock(map, nr, nc);

        v->row = nr;
        v->col = nc;

        if (v->route != NULL && v->route_idx < v->route_len) {
            if (v->row == v->route[v->route_idx] &&
                v->col == v->route[v->route_idx + 1]) {
                v->route_idx = (v->route_idx + 2) % v->route_len;
                ambulance_update_direction_to_target(v);
            }
        }
    }
    return NULL;
}
