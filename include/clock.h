#ifndef CLOCK_H
#define CLOCK_H

#include "types.h"

/* Inicializa o relógio global: tick = 0, mutex e cond */
void clock_init(SimClock *clk);

/* Thread do relógio — incrementa tick a cada TICK_MS ms e faz broadcast */
void *clock_thread(void *arg);

/* Bloqueia até o tick mudar (usa pthread_cond_wait — SEM busy-wait) */
void clock_wait_tick(SimClock *clk, long last_tick);

/* Retorna o tick atual de forma segura (com lock) */
long clock_get_tick(SimClock *clk);

/* Destrói mutex e cond do relógio */
void clock_destroy(SimClock *clk);

#endif
