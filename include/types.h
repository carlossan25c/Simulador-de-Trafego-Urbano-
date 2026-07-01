#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>
#include "constants.h"

/* Célula do mapa */
typedef struct {
    int type;        /* tipo da célula: rua, cruzamento, parede */
    int direction;   /* direção permitida: horizontal, vertical, mão única */
    int lane;        /* faixa (usado em vias de mão dupla) */
    int occupied;    /* 0 = livre, 1 = ocupada */
    int vehicle_id;  /* ID do veículo na célula (-1 se vazia) */
    pthread_mutex_t mutex;
} Cell;

/* Mapa da simulação */
typedef struct {
    Cell grid[MAP_ROWS][MAP_COLS];
    int  rows;
    int  cols;
} Map;

/* Veículo */
typedef struct {
    int id;
    int row, col;       /* posição atual no mapa */
    int direction;      /* direção atual do movimento */
    int speed;          /* velocidade: SPEED_FAST / MEDIUM / SLOW */
    int ticks_to_move;  /* contador regressivo até o próximo movimento */
    int active;         /* 1 = em movimento, 0 = parado */
    int *route;         /* rota fixa (somente ambulância) */
    int  route_len;
    int  route_idx;
    int  stuck_ticks;   /* ticks consecutivos sem conseguir mover */
} Vehicle;

/* Semáforo de trânsito */
typedef struct {
    int id;
    int intersection_row;
    int intersection_col;
    int state_horizontal; /* estado do sinal para a via horizontal */
    int state_vertical;   /* estado do sinal para a via vertical */
    int green_duration;   /* duração do verde em ticks */
    int red_duration;     /* duração do vermelho em ticks */
    int elapsed_ticks;       /* ticks transcorridos desde o último ciclo/reset */
    int priority_hold_ticks; /* >0: fase bloqueada por prioridade da ambulância */
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} TrafficLight;

/* Relógio global da simulação */
typedef struct {
    long            tick;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} SimClock;

#endif
