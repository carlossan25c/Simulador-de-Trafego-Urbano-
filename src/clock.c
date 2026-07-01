#include <time.h>
#include <pthread.h>
#include "clock.h"
#include "constants.h"

void clock_init(SimClock *clk)
{
    clk->tick = 0;
    pthread_mutex_init(&clk->mutex, NULL);
    pthread_cond_init(&clk->cond, NULL);
}

void clock_destroy(SimClock *clk)
{
    pthread_mutex_destroy(&clk->mutex);
    pthread_cond_destroy(&clk->cond);
}

void *clock_thread(void *arg)
{
    SimClock *clk = (SimClock *)arg;

    struct timespec ts = {
        .tv_sec  = TICK_MS / 1000,
        .tv_nsec = (TICK_MS % 1000) * 1000000L
    };

    while (1) {
        nanosleep(&ts, NULL);

        pthread_mutex_lock(&clk->mutex);
        clk->tick++;
        pthread_cond_broadcast(&clk->cond);
        pthread_mutex_unlock(&clk->mutex);
    }

    return NULL;
}

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
    while (clk->tick == last_tick)
        pthread_cond_wait(&clk->cond, &clk->mutex);
    pthread_mutex_unlock(&clk->mutex);
}
