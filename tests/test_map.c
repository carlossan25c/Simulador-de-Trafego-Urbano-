#include <stdio.h>
#include "map.h"
#include "constants.h"

static void print_map(Map *map)
{
    for (int r = 0; r < map->rows; r++) {
        for (int c = 0; c < map->cols; c++) {
            Cell *cell = &map->grid[r][c];
            switch (cell->type) {
                case CELL_INTERSECTION:
                    printf("+");
                    break;
                case CELL_ROAD:
                    if (cell->direction == DIR_HORIZONTAL)
                        printf("-");
                    else if (cell->direction == DIR_VERTICAL)
                        printf("|");
                    else if (cell->direction == DIR_EAST)
                        printf(">");
                    else
                        printf(".");
                    break;
                case CELL_WALL:
                default:
                    printf("#");
                    break;
            }
        }
        printf("\n");
    }
}

int main(void)
{
    Map map;

    printf("=== Teste: map_init ===\n\n");

    map_init(&map);
    print_map(&map);

    printf("\nLinhas: %d | Colunas: %d\n", map.rows, map.cols);
    printf("Teste concluido.\n");

    map_destroy(&map);
    return 0;
}
