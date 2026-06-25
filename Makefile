# Makefile for http-server

CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude -Ivendor/cJSON
LDFLAGS =

BUILD_DIR = build
TARGET    = $(BUILD_DIR)/http-server

SRCS = src/main.c \
       src/config.c \
       src/backend.c \
       src/proxy.c \
       vendor/cJSON/cJSON.c

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

TEST_TARGET = $(BUILD_DIR)/test_config
TEST_SRCS   = tests/test_config.c \
              src/config.c \
              vendor/cJSON/cJSON.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)
