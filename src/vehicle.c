#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "constants.h"
#include "vehicle.h"
#include "map.h"
#include "clock.h"
#include "traffic_light.h"

/* Tabelas de deslocamento por direção */
static const int DR[4] = {-1,  1,  0,  0};  /* N, S, E, W */
static const int DC[4] = { 0,  0,  1, -1};
static const int DIRS[4] = {DIR_NORTH, DIR_SOUTH, DIR_EAST, DIR_WEST};

/* Pontos de entrada do mapa — bordas de cada via */
static const struct { int row, col, dir; } SPAWN_POINTS[] = {
    {  5,  1, DIR_EAST  },  /* H1_EAST  — borda oeste  */
    {  6, 58, DIR_WEST  },  /* H1_WEST  — borda leste  */
    { 12,  1, DIR_EAST  },  /* H2_EAST  — borda oeste  */
    { 13, 58, DIR_WEST  },  /* H2_WEST  — borda leste  */
    { 19,  1, DIR_EAST  },  /* H3_EAST  — borda oeste  */
    {  1, 10, DIR_SOUTH },  /* V1_SOUTH — borda norte  */
    { 24, 11, DIR_NORTH },  /* V1_NORTH — borda sul    */
    {  1, 23, DIR_SOUTH },  /* V2_SOUTH — borda norte  */
    { 24, 24, DIR_NORTH },  /* V2_NORTH — borda sul    */
    {  1, 36, DIR_SOUTH },  /* V3_SOUTH — borda norte  */
    { 24, 37, DIR_NORTH },  /* V3_NORTH — borda sul    */
    {  1, 49, DIR_SOUTH },  /* V4_SOUTH — borda norte  */
    { 24, 50, DIR_NORTH },  /* V4_NORTH — borda sul    */
};

static inline int opposite(int dir) {
    switch (dir) {
        case DIR_NORTH: return DIR_SOUTH;
        case DIR_SOUTH: return DIR_NORTH;
        case DIR_EAST:  return DIR_WEST;
        default:        return DIR_EAST;
    }
}

/*
 * Escolhe direção aleatória a partir da célula atual.
 * Regra de mão: só entra em CELL_ROAD se a faixa destino tem a mesma direção
 * que o movimento — evita contramão.
 * Em CELL_INTERSECTION, qualquer direção é aceita (cruzamento não tem sentido).
 */
static int random_valid_dir(Vehicle *v, Map *map, int avoid_uturn)
{
    int opp = avoid_uturn ? opposite(v->direction) : -1;
    int valid[4], cnt = 0;

    for (int i = 0; i < 4; i++) {
        if (DIRS[i] == opp) continue;
        int nr = v->row + DR[i];
        int nc = v->col + DC[i];
        if (nr < 0 || nr >= MAP_ROWS || nc < 0 || nc >= MAP_COLS) continue;

        int t = map->grid[nr][nc].type;
        if (t == CELL_INTERSECTION) {
            valid[cnt++] = DIRS[i];
        } else if (t == CELL_ROAD && map->grid[nr][nc].direction == DIRS[i]) {
            /* Apenas entra na faixa se ela vai na mesma direção do movimento */
            valid[cnt++] = DIRS[i];
        }
    }

    if (cnt == 0) {
        if (avoid_uturn)
            return random_valid_dir(v, map, 0);
        return v->direction;
    }
    return valid[rand() % cnt];
}

/* Verifica o bloco 2×2 do semáforo */
static TrafficLight *find_light_at(TrafficLight *lights, int n_lights,
                                   int row, int col)
{
    for (int i = 0; i < n_lights; i++) {
        int lr = lights[i].intersection_row;
        int lc = lights[i].intersection_col;
        if ((row == lr || row == lr + 1) && (col == lc || col == lc + 1))
            return &lights[i];
    }
    return NULL;
}

/*
 * Libera a célula atual e reposiciona o veículo num ponto de entrada livre,
 * simulando que ele "saiu" do mapa e "entrou" por outra via.
 */
static void vehicle_respawn(Vehicle *v, Map *map)
{
    cell_lock(map, v->row, v->col);
    cell_release(map, v->row, v->col);
    cell_unlock(map, v->row, v->col);

    int n     = (int)(sizeof(SPAWN_POINTS) / sizeof(SPAWN_POINTS[0]));
    int start = rand() % n;

    for (int i = 0; i < n; i++) {
        int idx = (start + i) % n;
        int r   = SPAWN_POINTS[idx].row;
        int c   = SPAWN_POINTS[idx].col;

        cell_lock(map, r, c);
        if (cell_is_free(map, r, c)) {
            cell_occupy(map, r, c, v->id);
            cell_unlock(map, r, c);
            v->row       = r;
            v->col       = c;
            v->direction = SPAWN_POINTS[idx].dir;
            v->stuck_ticks = 0;
            return;
        }
        cell_unlock(map, r, c);
    }
    /* Todos os pontos lotados: permanece invisível até o próximo tick */
}

void vehicle_init(Vehicle *v, int id, int start_row, int start_col,
                  int speed, int direction)
{
    v->id            = id;
    v->row           = start_row;
    v->col           = start_col;
    v->direction     = direction;
    v->speed         = speed;
    v->ticks_to_move = speed;
    v->active        = 1;
    v->route         = NULL;
    v->route_len     = 0;
    v->route_idx     = 0;
    v->stuck_ticks   = 0;
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

void *vehicle_thread(void *arg)
{
    VehicleThreadArgs *args = (VehicleThreadArgs *)arg;
    Vehicle           *v   = args->vehicle;
    Map               *map = args->map;
    SimClock          *clk = args->clock;

    long last_tick = clock_get_tick(clk);

    while (v->active) {

        /* 1. Aguarda próximo tick */
        clock_wait_tick(clk, last_tick);
        last_tick = clock_get_tick(clk);

        /* 2. Lógica de velocidade */
        v->ticks_to_move--;
        if (v->ticks_to_move > 0)
            continue;
        v->ticks_to_move = v->speed;

        int cur_type = map->grid[v->row][v->col].type;

        /* 3. Em cruzamento: possivelmente escolhe nova saída */
        if (cur_type == CELL_INTERSECTION && rand() % 100 < TURN_PROB)
            v->direction = random_valid_dir(v, map, 1);

        /* 4. Calcula próxima célula na direção atual */
        int nr, nc;
        if (get_next_cell(v, &nr, &nc) != 0) {
            /* Fora do mapa: redireciona em cruzamento */
            if (cur_type == CELL_INTERSECTION)
                v->direction = random_valid_dir(v, map, 0);
            v->stuck_ticks = 0;
            continue;
        }

        int next_type = map->grid[nr][nc].type;

        /*
         * 5. Valida próxima célula conforme mão de direção:
         *    - CELL_INTERSECTION: sempre ok
         *    - CELL_ROAD a partir de cruzamento: só entra se faixa bate com o movimento
         *    - CELL_ROAD a partir de rua: ok (mesma faixa, sentido já fixado)
         *    - CELL_WALL: fim de via → veículo desaparece e reaparece em nova entrada
         */
        int next_ok;
        if (next_type == CELL_WALL) {
            next_ok = 0;
        } else if (next_type == CELL_INTERSECTION) {
            next_ok = 1;
        } else {
            next_ok = (cur_type == CELL_INTERSECTION)
                      ? (map->grid[nr][nc].direction == v->direction)
                      : 1;
        }

        if (!next_ok) {
            if (cur_type == CELL_INTERSECTION) {
                /* Saída inválida do cruzamento: reescolhe direção */
                v->direction = random_valid_dir(v, map, 1);
            } else {
                /* Fim de via: sai do mapa e reaparece por outra entrada */
                vehicle_respawn(v, map);
            }
            v->stuck_ticks = 0;
            continue;
        }

        /* 6. Verifica semáforo */
        TrafficLight *tl = find_light_at(args->lights, args->n_lights, nr, nc);
        if (tl != NULL)
            traffic_light_wait_green(tl, v->direction);

        /* 7. Adquire locks em ordem crescente de índice */
        int cur_idx  = v->row * MAP_COLS + v->col;
        int next_idx = nr     * MAP_COLS + nc;

        if (cur_idx < next_idx) {
            cell_lock(map, v->row, v->col);
            cell_lock(map, nr, nc);
        } else {
            cell_lock(map, nr, nc);
            cell_lock(map, v->row, v->col);
        }

        /* 8. Move se célula livre */
        if (!cell_is_free(map, nr, nc)) {
            cell_unlock(map, v->row, v->col);
            cell_unlock(map, nr, nc);

            v->stuck_ticks++;
            /* Só muda direção se estiver preso num cruzamento */
            if (v->stuck_ticks >= MAX_STUCK_TICKS && cur_type == CELL_INTERSECTION) {
                v->direction = random_valid_dir(v, map, 0);
                v->stuck_ticks = 0;
            }
            continue;
        }

        cell_occupy(map, nr, nc, v->id);
        cell_release(map, v->row, v->col);
        cell_unlock(map, v->row, v->col);
        cell_unlock(map, nr, nc);

        v->row = nr;
        v->col = nc;
        v->stuck_ticks = 0;

        /* 9. Ao sair de cruzamento para rua: sincroniza direção com o sentido da faixa */
        if (cur_type == CELL_INTERSECTION && map->grid[nr][nc].type == CELL_ROAD)
            v->direction = map->grid[nr][nc].direction;
    }

    return NULL;
}
