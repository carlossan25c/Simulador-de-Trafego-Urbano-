#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Tamanho do mapa */
#define MAP_ROWS 23
#define MAP_COLS 60

/* Número de veículos na simulação (inclui a ambulância) */
#define NUM_VEHICLES 15

/* Duração de cada tick em milissegundos */
#define TICK_MS 300

/* Velocidade dos veículos: quantos ticks entre cada movimento */
#define SPEED_FAST   1
#define SPEED_MEDIUM 2
#define SPEED_SLOW   4

/* Direções de movimento */
#define DIR_NORTH 0
#define DIR_SOUTH 1
#define DIR_EAST  2
#define DIR_WEST  3

/* ID reservado para a ambulância */
#define AMBULANCE_ID 0

/* Tipos de célula */
#define CELL_WALL         0
#define CELL_ROAD         1
#define CELL_INTERSECTION 2

/* Direções de via (cell.direction) — distintas das direções de movimento */
#define DIR_HORIZONTAL 4  /* mão dupla: leste e oeste permitidos */
#define DIR_VERTICAL   5  /* mão dupla: norte e sul permitidos */

#endif
