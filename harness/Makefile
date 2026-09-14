CC      = gcc
CFLAGS  = -Wall -Wextra -g -I./include -I../monitor/include -I../ipc
LDFLAGS = -pthread

BUILD_DIR   = build
SRC_DIR     = src
MONITOR_LIB = ../monitor/build/libmonitor.a
IPC_SRC     = ../ipc/fifo_channel.c

SCENARIOS = circular_wait circular_wait_fixed circular_wait_live \
            dining_philosophers dining_philosophers_fixed high_contention \
            circular_wait_ipc
BINARIES = $(addprefix $(BUILD_DIR)/, $(SCENARIOS))

.PHONY: all clean

all: $(BINARIES)

$(BUILD_DIR)/circular_wait: $(SRC_DIR)/scenario_circular_wait.c $(SRC_DIR)/common.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/circular_wait_fixed: $(SRC_DIR)/scenario_circular_wait_fixed.c $(SRC_DIR)/common.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/circular_wait_live: $(SRC_DIR)/scenario_circular_wait_live.c $(MONITOR_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(MONITOR_LIB) $(LDFLAGS)

$(BUILD_DIR)/dining_philosophers: $(SRC_DIR)/scenario_dining_philosophers.c $(SRC_DIR)/common.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/dining_philosophers_fixed: $(SRC_DIR)/scenario_dining_philosophers_fixed.c $(SRC_DIR)/common.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/high_contention: $(SRC_DIR)/scenario_high_contention.c $(SRC_DIR)/common.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/circular_wait_ipc: $(SRC_DIR)/scenario_circular_wait_ipc.c \
                                  $(SRC_DIR)/common.c \
                                  $(SRC_DIR)/ipc_worker.c \
                                  $(IPC_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
