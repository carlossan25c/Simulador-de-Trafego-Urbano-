#ifndef VEHICLE_H
#define VEHICLE_H

#include <stdlib.h>
#include "types.h"
#include "constants.h"

/* Struct de argumentos passados para a thread do veículo */
typedef struct {
    Vehicle      *vehicle;
    Map          *map;
    SimClock     *clock;
    TrafficLight *lights;
    int           n_lights;
} VehicleThreadArgs;

/* Inicializa veículo com posição, velocidade e direção inicial */
void vehicle_init(Vehicle *v, int id, int start_row, int start_col,
                  int speed, int direction);

/* Função da thread do veículo (assinatura obrigatória para pthread_create) */
void *vehicle_thread(void *arg);

/* Calcula célula adjacente na direção atual do veículo */
int get_next_cell(Vehicle *v, int *next_row, int *next_col);

/* Retorna 1 se o veículo é a ambulância */
int vehicle_is_ambulance(Vehicle *v);

#endif