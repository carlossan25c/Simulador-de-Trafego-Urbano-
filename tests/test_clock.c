#include <stdio.h>
#include <pthread.h>
#include "clock.h"
#include "constants.h"

#define TOTAL_TICKS 10

int main(void)
{
    SimClock clk;
    pthread_t tid;

    printf("=== Teste: clock ===\n");
    printf("Aguardando %d ticks de %dms cada...\n\n", TOTAL_TICKS, TICK_MS);

    clock_init(&clk);
    pthread_create(&tid, NULL, clock_thread, &clk);

    long last = 0;
    while (last < TOTAL_TICKS) {
        clock_wait_tick(&clk, last);
        last = clock_get_tick(&clk);
        printf("[tick %2ld]\n", last);
    }

    pthread_cancel(tid);
    pthread_join(tid, NULL);
    clock_destroy(&clk);

    printf("\nTeste concluido.\n");
    return 0;
}
