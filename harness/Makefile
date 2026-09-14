CC = gcc
CFLAGS = -Wall -Wextra -g -I./include -I../monitor/include
LDFLAGS = -pthread

BUILD_DIR = build
SRC_DIR = src
MONITOR_LIB = ../monitor/build/libmonitor.a

SCENARIOS = circular_wait circular_wait_fixed circular_wait_live
BINARIES = $(addprefix $(BUILD_DIR)/, $(SCENARIOS))

.PHONY: all clean

all: $(BINARIES)

$(BUILD_DIR)/circular_wait: $(SRC_DIR)/scenario_circular_wait.c $(SRC_DIR)/common.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/circular_wait_fixed: $(SRC_DIR)/scenario_circular_wait_fixed.c $(SRC_DIR)/common.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/circular_wait_live: $(SRC_DIR)/scenario_circular_wait_live.c $(MONITOR_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(MONITOR_LIB) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
