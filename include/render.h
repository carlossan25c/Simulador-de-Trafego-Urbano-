#ifndef RENDER_H
#define RENDER_H

#include "types.h"

/* Número máximo de linhas de log exibidas abaixo do mapa */
#define LOG_MAX_LINES 5

/*
 * render_frame:
 *   Limpa o terminal e redesenha o estado completo da simulação.
 *   Lê cada célula do mapa sob lock para evitar leitura de estado inconsistente.
 */
void render_frame(Map *map, Vehicle *vehicles, int n_vehicles,
                  TrafficLight *lights, int n_lights, long tick);

/*
 * render_log:
 *   Adiciona uma mensagem ao buffer de log exibido abaixo da legenda.
 *   Thread-safe: usa mutex interno.
 */
void render_log(const char *msg);

/*
 * render_init:
 *   Inicializa o mutex interno do log. Deve ser chamado antes de qualquer
 *   outra função de render.
 */
void render_init(void);

/*
 * render_destroy:
 *   Destrói o mutex interno do log.
 */
void render_destroy(void);

#endif /* RENDER_H */
