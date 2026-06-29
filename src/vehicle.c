#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "constants.h"
#include "vehicle.h"
#include "map.h"
#include "clock.h"
#include "traffic_light.h"

/* --------------------------------------------------------------------------
 * Funções auxiliares internas
 * -------------------------------------------------------------------------- */

/* Procura o semáforo cuja posição de cruzamento coincide com (row, col).
 * Retorna ponteiro para o TrafficLight ou NULL se não houver semáforo ali. */
static TrafficLight *find_light_at(TrafficLight *lights, int n_lights,
                                   int row, int col)
{
    for (int i = 0; i < n_lights; i++) {
        if (lights[i].intersection_row == row &&
            lights[i].intersection_col == col)
            return &lights[i];
    }
    return NULL;
}

/* --------------------------------------------------------------------------
 * Implementações públicas
 * -------------------------------------------------------------------------- */

void vehicle_init(Vehicle *v, int id, int start_row, int start_col,
                  int speed, int *route, int route_len)
{
    v->id            = id;
    v->row           = start_row;
    v->col           = start_col;
    v->direction     = DIR_EAST;   /* direção inicial padrão */
    v->speed         = speed;
    v->ticks_to_move = speed;      /* contador começa cheio */
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

    /* Verificar limites do mapa */
    if (*next_row < 0 || *next_row >= MAP_ROWS ||
        *next_col < 0 || *next_col >= MAP_COLS)
        return -1;

    return 0;
}

void *vehicle_thread(void *arg)
{
    VehicleThreadArgs *args = (VehicleThreadArgs *)arg;
    Vehicle           *v   = args->vehicle;
    Map               *map = args->map;
    SimClock          *clk = args->clock;

    long last_tick = clock_get_tick(clk);

    while (v->active) {

        /* 1. Dorme até o próximo tick — SEM busy-wait */
        clock_wait_tick(clk, last_tick);
        last_tick = clock_get_tick(clk);

        /* 2. Lógica de velocidade: só move quando contador chega a 0 */
        v->ticks_to_move--;
        if (v->ticks_to_move > 0)
            continue;
        v->ticks_to_move = v->speed;   /* reinicia contador */

        /* 3. Calcula próxima célula */
        int nr, nc;
        if (get_next_cell(v, &nr, &nc) != 0) {
            /* Fora do mapa ou direção inválida — aguarda próximo tick */
            continue;
        }

        /* 4. Verifica semáforo — bloqueia se vermelho, SEM busy-wait */
        TrafficLight *tl = find_light_at(args->lights, args->n_lights, nr, nc);
        if (tl != NULL) {
            traffic_light_wait_green(tl, v->direction);
        }

        /* 5. Prevenção de deadlock: adquire locks em ordem crescente de índice
         *    índice = row * MAP_COLS + col
         *    Garante que nunca haja ciclo no grafo de recursos. */
        int cur_idx  = v->row * MAP_COLS + v->col;
        int next_idx = nr     * MAP_COLS + nc;

        if (cur_idx < next_idx) {
            cell_lock(map, v->row, v->col);
            cell_lock(map, nr, nc);
        } else {
            cell_lock(map, nr, nc);
            cell_lock(map, v->row, v->col);
        }

        /* 6. Verificar e ocupar atomicamente dentro dos locks */
        if (!cell_is_free(map, nr, nc)) {
            /* Célula ocupada: libera tudo e tenta no próximo tick */
            cell_unlock(map, v->row, v->col);
            cell_unlock(map, nr, nc);
            continue;
        }

        cell_occupy(map, nr, nc, v->id);
        cell_release(map, v->row, v->col);

        cell_unlock(map, v->row, v->col);
        cell_unlock(map, nr, nc);

        /* 7. Atualiza posição */
        v->row = nr;
        v->col = nc;

        /* 8. Avança waypoint da rota se chegou no cruzamento esperado */
        if (v->route != NULL && v->route_idx < v->route_len) {
            if (v->row == v->route[v->route_idx] &&
                v->col == v->route[v->route_idx + 1]) {
                v->route_idx = (v->route_idx + 2) % v->route_len;
            }
        }
    }

    return NULL;
}