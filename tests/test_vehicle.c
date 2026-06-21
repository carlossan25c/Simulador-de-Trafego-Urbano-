/*
 * tests/test_vehicle.c
 *
 * Teste manual isolado do módulo de veículos.
 * Usa stubs de map, clock e traffic_light — não depende de nenhum
 * outro .c do projeto. Basta compilar com os stubs abaixo.
 *
 * Compilação (a partir da raiz do projeto):
 *   gcc -Wall -Wextra -pthread -g -I include \
 *       tests/test_vehicle.c \
 *       -o tests/test_vehicle && ./tests/test_vehicle
 *
 * Critério de conclusão:
 *   - 3 veículos se movendo por 20 ticks sem jamais ocupar a mesma célula
 *   - Log imprime posição de cada veículo a cada tick
 *   - Programa encerra limpo (sem travamento)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "types.h"
#include "constants.h"
#include "vehicle.h"

/* =========================================================================
 * STUB — Map
 * Grade 10x10 simples; todas as células começam livres.
 * ========================================================================= */

#define STUB_ROWS 10
#define STUB_COLS 10

static Map stub_map;

void map_stub_init(void)
{
    stub_map.rows = STUB_ROWS;
    stub_map.cols = STUB_COLS;
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            stub_map.grid[r][c].occupied   = 0;
            stub_map.grid[r][c].vehicle_id = -1;
            stub_map.grid[r][c].type       = 1; /* ROAD */
            stub_map.grid[r][c].direction  = DIR_EAST;
            pthread_mutex_init(&stub_map.grid[r][c].mutex, NULL);
        }
    }
}

void map_stub_destroy(void)
{
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++)
            pthread_mutex_destroy(&stub_map.grid[r][c].mutex);
}

/* Funções de map.h implementadas como stubs */
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

/* =========================================================================
 * STUB — SimClock
 * Tick avança a cada TICK_MS ms e acorda todas as threads com cond_broadcast.
 * ========================================================================= */

#define TOTAL_TICKS 20

static SimClock stub_clock;
static int      clock_running = 1;

void clock_stub_init(void)
{
    stub_clock.tick = 0;
    pthread_mutex_init(&stub_clock.mutex, NULL);
    pthread_cond_init(&stub_clock.cond, NULL);
}

void clock_stub_destroy(void)
{
    pthread_mutex_destroy(&stub_clock.mutex);
    pthread_cond_destroy(&stub_clock.cond);
}

/* Funções de clock.h implementadas como stubs */
long clock_get_tick(SimClock *clk)
{
    pthread_mutex_lock(&clk->mutex);
    long t = clk->tick;
    pthread_mutex_unlock(&clk->mutex);
    return t;
}

void clock_wait_tick(SimClock *clk, long last_tick)
{
    pthread_mutex_lock(&clk->mutex);
    while (clk->tick == last_tick && clock_running)
        pthread_cond_wait(&clk->cond, &clk->mutex);
    pthread_mutex_unlock(&clk->mutex);
}

/* Thread do relógio stub */
static void *clock_thread_stub(void *arg)
{
    SimClock *clk = (SimClock *)arg;
    for (int i = 0; i < TOTAL_TICKS; i++) {
        usleep(TICK_MS * 1000);
        pthread_mutex_lock(&clk->mutex);
        clk->tick++;
        pthread_cond_broadcast(&clk->cond);
        pthread_mutex_unlock(&clk->mutex);
    }
    /* Sinaliza encerramento */
    pthread_mutex_lock(&clk->mutex);
    clock_running = 0;
    pthread_cond_broadcast(&clk->cond);
    pthread_mutex_unlock(&clk->mutex);
    return NULL;
}

/* =========================================================================
 * STUB — TrafficLight
 * Sempre retorna verde — o foco do teste é movimento e deadlock, não sinal.
 * ========================================================================= */

void traffic_light_wait_green(TrafficLight *tl, int direction)
{
    /* stub: sinal sempre verde, não bloqueia */
    (void)tl; (void)direction;
}

int traffic_light_is_green(TrafficLight *tl, int direction)
{
    (void)tl; (void)direction;
    return 1;
}

/* =========================================================================
 * Verificação de colisão
 * Percorre todos os pares de veículos e checa se algum ocupa a mesma célula.
 * ========================================================================= */

#define NUM_TEST_VEHICLES 3

static void check_collisions(Vehicle *vehicles, int n, long tick)
{
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (vehicles[i].row == vehicles[j].row &&
                vehicles[i].col == vehicles[j].col) {
                printf("\n[ERRO] COLISAO detectada no tick %ld: "
                       "veiculo %d e veiculo %d na celula (%d,%d)!\n",
                       tick, vehicles[i].id, vehicles[j].id,
                       vehicles[i].row, vehicles[i].col);
            }
        }
    }
}

/* =========================================================================
 * Thread de monitoramento
 * Imprime posições a cada tick e verifica colisões.
 * ========================================================================= */

typedef struct {
    Vehicle *vehicles;
    int      n;
    SimClock *clk;
} MonitorArgs;

static void *monitor_thread(void *arg)
{
    MonitorArgs *m    = (MonitorArgs *)arg;
    long         last = 0;

    while (clock_running || clock_get_tick(m->clk) > last) {
        clock_wait_tick(m->clk, last);
        last = clock_get_tick(m->clk);
        if (!clock_running && last == 0) break;

        printf("[tick %2ld]", last);
        for (int i = 0; i < m->n; i++)
            printf("  V%d=(%d,%d)", m->vehicles[i].id,
                   m->vehicles[i].row, m->vehicles[i].col);
        printf("\n");

        check_collisions(m->vehicles, m->n, last);
    }
    return NULL;
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    printf("=== Teste isolado: modulo vehicle ===\n");
    printf("Veiculos: %d | Ticks: %d | TICK_MS: %d\n\n",
           NUM_TEST_VEHICLES, TOTAL_TICKS, TICK_MS);

    /* 1. Inicializa mapa stub */
    map_stub_init();

    /* 2. Inicializa relógio stub */
    clock_stub_init();

    /* 3. Cria veículos em posições distintas com rotas simples (circulares)
     *    Todos na linha 1, colunas 0/2/4 — espaço suficiente para não colidir
     *    na largura do stub (STUB_COLS=10). */
    Vehicle vehicles[NUM_TEST_VEHICLES];

    /* V1: rápido, vai para leste */
    vehicle_init(&vehicles[0], 1, 1, 0, SPEED_FAST, NULL, 0);
    vehicles[0].direction = DIR_EAST;
    cell_occupy(&stub_map, 1, 0, vehicles[0].id);

    /* V2: médio, vai para leste */
    vehicle_init(&vehicles[1], 2, 1, 2, SPEED_MEDIUM, NULL, 0);
    vehicles[1].direction = DIR_EAST;
    cell_occupy(&stub_map, 1, 2, vehicles[1].id);

    /* V3: lento, vai para leste */
    vehicle_init(&vehicles[2], 3, 1, 4, SPEED_SLOW, NULL, 0);
    vehicles[2].direction = DIR_EAST;
    cell_occupy(&stub_map, 1, 4, vehicles[2].id);

    /* 4. Monta argumentos das threads de veículos */
    VehicleThreadArgs vargs[NUM_TEST_VEHICLES];
    for (int i = 0; i < NUM_TEST_VEHICLES; i++) {
        vargs[i].vehicle  = &vehicles[i];
        vargs[i].map      = &stub_map;
        vargs[i].clock    = &stub_clock;
        vargs[i].lights   = NULL;
        vargs[i].n_lights = 0;
    }

    /* 5. Lança thread do relógio */
    pthread_t clk_tid;
    pthread_create(&clk_tid, NULL, clock_thread_stub, &stub_clock);

    /* 6. Lança thread de monitoramento */
    MonitorArgs margs = { vehicles, NUM_TEST_VEHICLES, &stub_clock };
    pthread_t mon_tid;
    pthread_create(&mon_tid, NULL, monitor_thread, &margs);

    /* 7. Lança threads dos veículos */
    pthread_t vtids[NUM_TEST_VEHICLES];
    for (int i = 0; i < NUM_TEST_VEHICLES; i++)
        pthread_create(&vtids[i], NULL, vehicle_thread, &vargs[i]);

    /* 8. Aguarda relógio terminar e encerra veículos */
    pthread_join(clk_tid, NULL);

    for (int i = 0; i < NUM_TEST_VEHICLES; i++)
        vehicles[i].active = 0;

    /* Acorda threads que possam estar dormindo no cond_wait */
    pthread_mutex_lock(&stub_clock.mutex);
    pthread_cond_broadcast(&stub_clock.cond);
    pthread_mutex_unlock(&stub_clock.mutex);

    for (int i = 0; i < NUM_TEST_VEHICLES; i++)
        pthread_join(vtids[i], NULL);

    pthread_join(mon_tid, NULL);

    /* 9. Resultado final */
    printf("\n=== Posicoes finais ===\n");
    for (int i = 0; i < NUM_TEST_VEHICLES; i++)
        printf("  Veiculo %d: (%d, %d)\n",
               vehicles[i].id, vehicles[i].row, vehicles[i].col);

    printf("\nTeste concluido sem travamento.\n");

    /* 10. Limpeza */
    map_stub_destroy();
    clock_stub_destroy();

    return 0;
}