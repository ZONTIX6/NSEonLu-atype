CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
INCLUDES = -I../../modules/algorithm -I../../modules/chassis_force
SOURCES = test_force.c $(wildcard ../../modules/chassis_force/*.c)
.PHONY: test
.DEFAULT_GOAL := test_force.exe
BOARD_INCLUDES = -I../vehicle_motion/stubs -I../chassis_motion/stubs -I../dji_chassis_power_rtt/stubs -I../../application -I../../modules/imu -I../../modules/master_machine -I../../modules/chassis_power -I../../modules/telemetry -I../../modules/super_cap -I../../modules/power_meter -I../../modules/referee
test_board_config.exe: test_board_config.c ../../application/board/board_chassis_force.c ../../modules/chassis_force/chassis_force_config.c ../../modules/algorithm/chassis_power_transient.c ../../modules/algorithm/power_model.c
	$(CC) $(filter-out -pedantic,$(CFLAGS)) -ffunction-sections -fdata-sections -DCHASSIS_CONTROL_MODE=1 -DCHASSIS_FORCE_CALIBRATION_VALID=1 $(BOARD_INCLUDES) $(INCLUDES) $^ -Wl,--gc-sections -lm -o $@
test_force.exe: $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) -lm -o $@
POWER_INCLUDES = -I../dji_chassis_power_rtt/stubs -I../../modules/chassis_power -I../../modules/telemetry -I../../modules/super_cap -I../../modules/power_meter -I../../modules/referee
POWER_SOURCES = $(wildcard ../../modules/chassis_power/*.c) ../../modules/algorithm/power_model.c ../../modules/algorithm/chassis_power_transient.c ../../modules/algorithm/chassis_power_budget.c ../../modules/algorithm/chassis_power_outer_loop.c
test_energy.exe: test_energy.c $(POWER_SOURCES) $(filter-out test_force.c,$(SOURCES)) ../../modules/telemetry/chassis_force_telemetry.c
	$(CC) $(CFLAGS) -DCHASSIS_CONTROL_MODE=1 $(INCLUDES) $(POWER_INCLUDES) $^ -lm -o $@
ADAPTER_INCLUDES = -I../../modules/motor/DJImotor -I../../modules/motor -I../../modules/vehicle_safety -I../../application -I../../Inc
ADAPTER_SOURCES = ../../modules/telemetry/rtt_telemetry.c ../../modules/telemetry/runtime_tuning.c ../../modules/telemetry/pid_autotune.c ../../modules/telemetry/autotune_protocol.c
test_adapter.exe: test_adapter.c $(filter-out ../../modules/chassis_power/chassis_power.c,$(POWER_SOURCES)) $(filter-out test_force.c,$(SOURCES)) $(ADAPTER_SOURCES) ../../modules/motor/DJImotor/dji_force_adapter.c ../../modules/motor/DJImotor/dji_motor.c
	$(CC) $(filter-out -pedantic,$(CFLAGS)) -DCHASSIS_CONTROL_MODE=1 -ffunction-sections -fdata-sections $(POWER_INCLUDES) $(INCLUDES) $(ADAPTER_INCLUDES) $(filter-out ../../modules/motor/DJImotor/dji_force_adapter.c ../../modules/motor/DJImotor/dji_motor.c,$^) -Wl,--gc-sections -lm -o $@
test: test_force.exe test_energy.exe test_adapter.exe test_board_config.exe
	./test_force.exe
	./test_energy.exe
	./test_adapter.exe
	./test_board_config.exe
