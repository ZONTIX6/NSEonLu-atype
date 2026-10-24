CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
INCLUDES := -I../dji_chassis_power_rtt/stubs  -I../../modules/chassis_power -I../../modules/algorithm -I../../modules/telemetry -I../../modules/super_cap -I../../modules/power_meter -I../../modules/referee
SOURCE := test_chassis_power_module.c $(wildcard ../../modules/chassis_power/*.c) ../../modules/algorithm/power_model.c ../../modules/algorithm/chassis_power_transient.c ../../modules/algorithm/chassis_power_budget.c ../../modules/algorithm/chassis_power_outer_loop.c
TARGET := test_chassis_power_module.exe
.PHONY: all test
all: $(TARGET)
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -lm -o $@
test_spin_policy.exe: test_spin_policy.c $(filter-out test_chassis_power_module.c,$(SOURCE)) ../../modules/telemetry/rtt_telemetry.c ../../modules/algorithm/chassis_spin.c
	$(CC) $(CFLAGS) $(INCLUDES) -DCHASSIS_SPIN_MODE=1 $^ -lm -o $@
test: $(TARGET) test_spin_policy.exe
	./$(TARGET)
	./test_spin_policy.exe
	./test_spin_policy.exe direct50
	./test_spin_policy.exe biased

SOURCE += ../../modules/telemetry/rtt_telemetry.c
