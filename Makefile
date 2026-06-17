CC     = gcc
CFLAGS = -Wall -Wextra -pthread -g -I include

# Quando os arquivos .c forem criados pela equipe, compilar com:
#   make

SRC_DIR = src
OBJ_DIR = build
TARGET  = simulador

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -pthread

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
