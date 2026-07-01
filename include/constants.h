#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Tamanho do mapa */
#define MAP_ROWS 26
#define MAP_COLS 60

/* Número de veículos na simulação (inclui a ambulância) */
#define NUM_VEHICLES 15

/* Duração de cada tick em milissegundos */
#define TICK_MS 300

/* Velocidade dos veículos: quantos ticks entre cada movimento */
#define SPEED_FAST   1
#define SPEED_MEDIUM 2
#define SPEED_SLOW   4

/* Número de cruzamentos com semáforo (H1 e H2, 4 cada = 8 total) */
#define NUM_INTERSECTIONS 8

/* Direções de movimento e de faixa */
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

/*
 * Layout do mapa com faixas de mão dupla (2 células por sentido):
 *
 * Ruas horizontais:
 *   H1 (mão dupla): faixa leste = row 5, faixa oeste = row 6
 *   H2 (mão dupla): faixa leste = row 12, faixa oeste = row 13
 *   H3 (mão única → leste): row 19
 *
 * Ruas verticais (mão dupla, 2 colunas cada):
 *   V1: faixa sul = col 10, faixa norte = col 11
 *   V2: faixa sul = col 23, faixa norte = col 24
 *   V3: faixa sul = col 36, faixa norte = col 37
 *   V4: faixa sul = col 49, faixa norte = col 50
 *
 * Cruzamentos H1/H2 × V1-V4: blocos 2×2 células.
 */
#define H1_EAST  5
#define H1_WEST  6
#define H2_EAST 12
#define H2_WEST 13
#define H3_EAST 19

#define V1_SOUTH 10
#define V1_NORTH 11
#define V2_SOUTH 23
#define V2_NORTH 24
#define V3_SOUTH 36
#define V3_NORTH 37
#define V4_SOUTH 49
#define V4_NORTH 50

/* Navegação aleatória dos veículos */
#define TURN_PROB      35   /* % de chance de virar ao entrar em cruzamento */
#define MAX_STUCK_TICKS 5  /* ticks parado antes de forçar nova direção */

#endif
