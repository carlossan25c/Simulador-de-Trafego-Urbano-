#ifndef RENDER_H
#define RENDER_H

#include "types.h"

void render_frame(Map *map, Vehicle *vehicles, int n_vehicles,
                  TrafficLight *lights, int n_lights, long tick);
void render_log(const char *msg);

#endif
