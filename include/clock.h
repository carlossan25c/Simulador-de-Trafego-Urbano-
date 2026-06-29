#ifndef CLOCK_H
#define CLOCK_H

#include "types.h"

void  clock_init(SimClock *clk);
void  clock_destroy(SimClock *clk);
void *clock_thread(void *arg);

long  clock_get_tick(SimClock *clk);
void  clock_wait_tick(SimClock *clk, long last_tick);

#endif


