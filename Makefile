CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -Ivendor/cJSON
SRC = src/main.c src/backend.c src/proxy.c src/config.c vendor/cJSON/cJSON.c
BIN = proxy

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)
