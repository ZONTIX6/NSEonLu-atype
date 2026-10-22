CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
INCLUDES := -I../dji_chassis_power_rtt/stubs  -I../../modules/chassis_power -I../../modules/algorithm -I../../modules/telemetry -I../../modules/super_cap -I../../modules/power_meter -I../../modules/referee
SOURCE := test_chassis_power_module.c $(wildcard ../../modules/chassis_power/*.c) ../../modules/algorithm/power_model.c ../../modules/algorithm/chassis_power_transient.c ../../modules/algorithm/chassis_power_budget.c ../../modules/algorithm/chassis_power_outer_loop.c
TARGET := test_chassis_power_module.exe
.PHONY: all test
all: $(TARGET)
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -lm -o $@
test: $(TARGET)
	./$(TARGET)

SOURCE += ../../modules/telemetry/rtt_telemetry.c
