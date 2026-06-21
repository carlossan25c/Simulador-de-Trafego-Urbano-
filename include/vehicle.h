#ifndef VEHICLE_H
#define VEHICLE_H

#include <stdlib.h>
#include "types.h"
#include "constants.h"

/* Constantes de direção */
#define DIR_NORTH 0
#define DIR_SOUTH 1
#define DIR_EAST  2
#define DIR_WEST  3

/* ID reservado para ambulância */
#define AMBULANCE_ID 0

/* Struct de argumentos passados para a thread do veículo */
typedef struct {
    Vehicle      *vehicle;
    Map          *map;
    SimClock     *clock;
    TrafficLight *lights;
    int           n_lights;
} VehicleThreadArgs;

/* Inicializa veículo com id, posição, velocidade e rota */
void vehicle_init(Vehicle *v, int id, int start_row, int start_col,
                  int speed, int *route, int route_len);

/* Função da thread do veículo (assinatura obrigatória para pthread_create) */
void *vehicle_thread(void *arg);

/* Calcula célula adjacente na direção atual do veículo */
int get_next_cell(Vehicle *v, int *next_row, int *next_col);

/* Retorna 1 se o veículo é a ambulância */
int vehicle_is_ambulance(Vehicle *v);

#endif