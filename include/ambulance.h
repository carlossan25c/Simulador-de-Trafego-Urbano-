#ifndef AMBULANCE_H
#define AMBULANCE_H

#include "types.h"

void  ambulance_init(Vehicle *amb);
void  ambulance_request_priority(TrafficLight *tl, int direction);
void *ambulance_thread(void *arg);

#endif
