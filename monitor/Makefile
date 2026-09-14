CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
AR = ar

BUILD_DIR = build
SRC_DIR = src

OBJS = $(BUILD_DIR)/graph.o $(BUILD_DIR)/cycle_detector.o
LIB = $(BUILD_DIR)/libmonitor.a

.PHONY: all clean

all: $(LIB)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
