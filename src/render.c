#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "render.h"
#include "types.h"
#include "constants.h"

/* ─── Códigos de cor ANSI ─────────────────────────────────────────────────── */
#define ANSI_RESET        "\033[0m"
#define ANSI_GRAY         "\033[90m"
#define ANSI_WHITE        "\033[37m"
#define ANSI_CYAN         "\033[36m"
#define ANSI_RED_BRIGHT   "\033[91m"
#define ANSI_GREEN        "\033[32m"
#define ANSI_RED          "\033[31m"
#define ANSI_YELLOW       "\033[33m"

/* ─── Tipos de célula ─────────────────────────────────────────────────────── */
#define CELL_WALL         0
#define CELL_ROAD_H       1
#define CELL_ROAD_V       2
#define CELL_INTERSECTION 3
#define CELL_ONE_WAY_N    4
#define CELL_ONE_WAY_S    5
#define CELL_ONE_WAY_E    6
#define CELL_ONE_WAY_W    7

/* ─── ID especial da ambulância ───────────────────────────────────────────── */
#ifndef AMBULANCE_ID
#define AMBULANCE_ID 0
#endif

/* ─── Buffer de log ───────────────────────────────────────────────────────── */
static char     log_buf[LOG_MAX_LINES][256];
static int      log_count = 0;
static pthread_mutex_t log_mutex;

/* ─── Buffer de frame ─────────────────────────────────────────────────────── */
static char frame_buf[8192];
static int  frame_pos;

static void fb_append(const char *s) {
    int len = (int)strlen(s);
    if (frame_pos + len < (int)sizeof(frame_buf) - 1) {
        memcpy(frame_buf + frame_pos, s, len);
        frame_pos += len;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * render_init / render_destroy
 * ═══════════════════════════════════════════════════════════════════════════ */
void render_init(void) {
    pthread_mutex_init(&log_mutex, NULL);
    memset(log_buf, 0, sizeof(log_buf));
    log_count = 0;
}

void render_destroy(void) {
    pthread_mutex_destroy(&log_mutex);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * render_log  — thread-safe
 * ═══════════════════════════════════════════════════════════════════════════ */
void render_log(const char *msg) {
    pthread_mutex_lock(&log_mutex);

    if (log_count < LOG_MAX_LINES) {
        strncpy(log_buf[log_count], msg, 255);
        log_buf[log_count][255] = '\0';
        log_count++;
    } else {
        for (int i = 0; i < LOG_MAX_LINES - 1; i++)
            strncpy(log_buf[i], log_buf[i + 1], 255);
        strncpy(log_buf[LOG_MAX_LINES - 1], msg, 255);
        log_buf[LOG_MAX_LINES - 1][255] = '\0';
    }

    pthread_mutex_unlock(&log_mutex);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Helpers internos
 * ═══════════════════════════════════════════════════════════════════════════ */
static int vehicle_at(Vehicle *vehicles, int n_vehicles, int row, int col) {
    for (int i = 0; i < n_vehicles; i++)
        if (vehicles[i].active && vehicles[i].row == row && vehicles[i].col == col)
            return vehicles[i].id;
    return -1;
}

static TrafficLight *light_at(TrafficLight *lights, int n_lights, int row, int col) {
    for (int i = 0; i < n_lights; i++)
        if (lights[i].intersection_row == row && lights[i].intersection_col == col)
            return &lights[i];
    return NULL;
}

static void cell_snapshot(Map *map, int r, int c, int *type, int *direction) {
    pthread_mutex_lock(&map->grid[r][c].mutex);
    *type      = map->grid[r][c].type;
    *direction = map->grid[r][c].direction;
    pthread_mutex_unlock(&map->grid[r][c].mutex);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * render_frame
 * ═══════════════════════════════════════════════════════════════════════════ */
void render_frame(Map *map, Vehicle *vehicles, int n_vehicles,
                  TrafficLight *lights, int n_lights, long tick)
{
    char tmp[64];

    frame_pos = 0;
    memset(frame_buf, 0, sizeof(frame_buf));

    /* Limpa terminal sem piscar */
    fb_append("\033[H\033[J");

    /* Cabeçalho */
    fb_append(ANSI_WHITE);
    fb_append("╔══════════════════════════════════════════════════════════╗\n");
    fb_append("║         SIMULADOR DE TRÁFEGO URBANO — SO/Pthreads       ║\n");
    fb_append("╚══════════════════════════════════════════════════════════╝\n");
    fb_append(ANSI_RESET);

    /* Mapa célula por célula */
    for (int r = 0; r < map->rows; r++) {
        for (int c = 0; c < map->cols; c++) {

            /* Veículo? */
            int vid = vehicle_at(vehicles, n_vehicles, r, c);
            if (vid >= 0) {
                if (vid == AMBULANCE_ID) {
                    fb_append(ANSI_RED_BRIGHT);
                    fb_append("A");
                } else {
                    fb_append(ANSI_CYAN);
                    snprintf(tmp, sizeof(tmp), "%d", vid % 10);
                    fb_append(tmp);
                }
                fb_append(ANSI_RESET);
                continue;
            }

            /* Semáforo? */
            TrafficLight *tl = light_at(lights, n_lights, r, c);
            if (tl != NULL) {
                pthread_mutex_lock(&tl->mutex);
                int sh = tl->state_horizontal;
                pthread_mutex_unlock(&tl->mutex);

                fb_append(sh ? ANSI_GREEN : ANSI_RED);
                fb_append(sh ? "G" : "R");
                fb_append(ANSI_RESET);
                continue;
            }

            /* Célula comum */
            int type, direction;
            cell_snapshot(map, r, c, &type, &direction);

            switch (type) {
                case CELL_WALL:
                    fb_append(ANSI_GRAY);  fb_append("#");  break;
                case CELL_ROAD_H:
                    fb_append(ANSI_WHITE); fb_append("\xe2\x94\x80"); break; /* ─ */
                case CELL_ROAD_V:
                    fb_append(ANSI_WHITE); fb_append("\xe2\x94\x82"); break; /* │ */
                case CELL_INTERSECTION:
                    fb_append(ANSI_WHITE); fb_append("+");  break;
                case CELL_ONE_WAY_N:
                    fb_append(ANSI_YELLOW); fb_append("\xe2\x86\x91"); break; /* ↑ */
                case CELL_ONE_WAY_S:
                    fb_append(ANSI_YELLOW); fb_append("\xe2\x86\x93"); break; /* ↓ */
                case CELL_ONE_WAY_E:
                    fb_append(ANSI_YELLOW); fb_append("\xe2\x86\x92"); break; /* → */
                case CELL_ONE_WAY_W:
                    fb_append(ANSI_YELLOW); fb_append("\xe2\x86\x90"); break; /* ← */
                default:
                    fb_append(ANSI_GRAY);  fb_append(" ");  break;
            }
            fb_append(ANSI_RESET);
        }
        fb_append("\n");
    }

    /* Legenda */
    fb_append(ANSI_WHITE);
    fb_append("──────────────────────────────────────────────────────────\n");

    int ativos = 0;
    for (int i = 0; i < n_vehicles; i++)
        if (vehicles[i].active) ativos++;

    snprintf(tmp, sizeof(tmp), "  Tick: %ld   |   Veículos ativos: %d/%d\n", tick, ativos, n_vehicles);
    fb_append(tmp);

    fb_append("  Semáforos: ");
    for (int i = 0; i < n_lights; i++) {
        pthread_mutex_lock(&lights[i].mutex);
        int sh = lights[i].state_horizontal;
        int sv = lights[i].state_vertical;
        pthread_mutex_unlock(&lights[i].mutex);

        snprintf(tmp, sizeof(tmp), "[S%d H:%s V:%s] ",
                 lights[i].id, sh ? "G" : "R", sv ? "G" : "V");
        fb_append(tmp);
    }
    fb_append("\n");

    fb_append("  ");
    fb_append(ANSI_GRAY);      fb_append("# parede  ");
    fb_append(ANSI_WHITE);     fb_append("─│ via  + cruzamento  ");
    fb_append(ANSI_YELLOW);    fb_append("→↑ mão única  ");
    fb_append(ANSI_CYAN);      fb_append("0-9 carro  ");
    fb_append(ANSI_RED_BRIGHT);fb_append("A ambulância  ");
    fb_append(ANSI_GREEN);     fb_append("G verde  ");
    fb_append(ANSI_RED);       fb_append("R vermelho");
    fb_append(ANSI_RESET);
    fb_append("\n");

    fb_append("──────────────────────────────────────────────────────────\n");
    fb_append(ANSI_RESET);

    /* Log de eventos */
    pthread_mutex_lock(&log_mutex);
    for (int i = 0; i < log_count; i++) {
        fb_append("  > ");
        fb_append(log_buf[i]);
        fb_append("\n");
    }
    pthread_mutex_unlock(&log_mutex);

    /* Flush único */
    fwrite(frame_buf, 1, frame_pos, stdout);
    fflush(stdout);
}