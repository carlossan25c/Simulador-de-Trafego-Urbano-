#include <stdio.h>
#include <pthread.h>
#include "types.h"
#include "constants.h"

#ifdef _WIN32
#include <windows.h>
#define msleep(ms) Sleep(ms)
#else
#include <unistd.h>
#define msleep(ms) usleep((ms) * 1000)
#endif

void  clock_init(SimClock *clk);
void *clock_thread(void *arg);
void  clock_wait_tick(SimClock *clk, long last_tick);
long  clock_get_tick(SimClock *clk);
void  clock_destroy(SimClock *clk);

void clock_init(SimClock *clk)
{
    clk->tick = 0;
    pthread_mutex_init(&clk->mutex, NULL);
    pthread_cond_init(&clk->cond, NULL);
}

void *clock_thread(void *arg)
{
    SimClock *clk = (SimClock *)arg;
    while (1) {
        msleep(TICK_MS);
        pthread_mutex_lock(&clk->mutex);
        clk->tick++;
        pthread_cond_broadcast(&clk->cond);
        pthread_mutex_unlock(&clk->mutex);
    }
    return NULL;
}

void clock_wait_tick(SimClock *clk, long last_tick)
{
    pthread_mutex_lock(&clk->mutex);
    while (clk->tick == last_tick) {
        pthread_cond_wait(&clk->cond, &clk->mutex);
    }
    pthread_mutex_unlock(&clk->mutex);
}

long clock_get_tick(SimClock *clk)
{
    long t;
    pthread_mutex_lock(&clk->mutex);
    t = clk->tick;
    pthread_mutex_unlock(&clk->mutex);
    return t;
}

void clock_destroy(SimClock *clk)
{
    pthread_mutex_destroy(&clk->mutex);
    pthread_cond_destroy(&clk->cond);
}
