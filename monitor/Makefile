CC      = gcc
CFLAGS  = -Wall -Wextra -g -I./include -I../ipc
AR      = ar

BUILD_DIR = build
SRC_DIR   = src
IPC_DIR   = ../ipc

# Library objects (Phase 1-4 reusable core)
LIB_OBJS = $(BUILD_DIR)/graph.o \
            $(BUILD_DIR)/cycle_detector.o \
            $(BUILD_DIR)/feature_extractor.o
LIB = $(BUILD_DIR)/libmonitor.a

# Monitor daemon objects (Phase 6-7)
DAEMON_OBJS = $(BUILD_DIR)/main.o \
              $(BUILD_DIR)/ipc_reader.o \
              $(BUILD_DIR)/resolver.o \
              $(BUILD_DIR)/fifo_channel.o
DAEMON = $(BUILD_DIR)/monitor

.PHONY: all lib daemon clean

all: lib daemon

lib: $(LIB)

daemon: $(DAEMON)

# Library
$(BUILD_DIR)/graph.o: $(SRC_DIR)/graph.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/cycle_detector.o: $(SRC_DIR)/cycle_detector.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/feature_extractor.o: $(SRC_DIR)/feature_extractor.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^

# IPC object (compiled here for the daemon)
$(BUILD_DIR)/fifo_channel.o: $(IPC_DIR)/fifo_channel.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Daemon sources
$(BUILD_DIR)/ipc_reader.o: $(SRC_DIR)/ipc_reader.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/resolver.o: $(SRC_DIR)/resolver.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(DAEMON): $(DAEMON_OBJS) $(LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(DAEMON_OBJS) $(LIB)

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
