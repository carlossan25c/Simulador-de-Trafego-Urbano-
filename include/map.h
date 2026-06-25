#ifndef MAP_H
#define MAP_H

#include "types.h"

/* Inicializa o mapa com o layout definido no pré-requisito coletivo */
void map_init(Map *map);

/* Wrappers para pthread_mutex_lock/unlock da célula específica */
void cell_lock(Map *map, int row, int col);
void cell_unlock(Map *map, int row, int col);

/* Retorna 1 se a célula está livre (deve ser chamado DENTRO de um lock) */
int cell_is_free(Map *map, int row, int col);

/* Marca célula como ocupada pelo veículo */
void cell_occupy(Map *map, int row, int col, int vehicle_id);

/* Marca célula como livre */
void cell_release(Map *map, int row, int col);

/* Destrói todos os mutexes das células */
void map_destroy(Map *map);

#endif
